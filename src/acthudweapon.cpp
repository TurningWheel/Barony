/*-------------------------------------------------------------------------------

	BARONY
	File: acthudweapon.cpp
	Desc: behavior function for hud weapon (first person models)

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "items.hpp"
#include "sound.hpp"
#include "magic/magic.hpp"
#include "interface/interface.hpp"
#include "net.hpp"
#include "collision.hpp"
#include "player.hpp"
#include "scores.hpp"

Entity* hudweapon = NULL;
Entity* hudarm = NULL;
bool weaponSwitch = false;
bool shieldSwitch = false;

Sint32 throwGimpTimer = 0; // player cannot throw objects unless zero
Sint32 pickaxeGimpTimer = 0; // player cannot swap weapons immediately after using pickaxe 
							 // due to multiplayer weapon degrade lag... equipping new weapon before degrade
							// message hits can degrade the wrong weapon.
Sint32 swapWeaponGimpTimer = 0; // player cannot swap weapons unless zero

/*-------------------------------------------------------------------------------

	act*

	The following function describes an entity behavior. The function
	takes a pointer to the entity that uses it as an argument.

-------------------------------------------------------------------------------*/

void actHudArm(Entity* my)
{
	hudarm = my;
	Entity* parent = hudweapon;

	if (players[clientnum] == nullptr || players[clientnum]->entity == nullptr)
	{
		hudarm = nullptr;
		list_RemoveNode(my->mynode);
		return;
	}

	if (stats[clientnum]->HP <= 0)
	{
		hudarm = nullptr;
		list_RemoveNode(my->mynode);
		return;
	}

	if (parent == nullptr)
	{
		hudarm = nullptr;
		list_RemoveNode(my->mynode);
		return;
	}

	// sprite
	my->scalex = 1.f;
	my->scaley = 1.f;
	my->scalez = 1.f;
	// position
	my->x = parent->x;
	my->y = parent->y;
	my->z = parent->z - 2.5;

	Monster playerRace = players[clientnum]->entity->getMonsterFromPlayerRace(stats[clientnum]->playerRace);
	int playerAppearance = stats[clientnum]->appearance;
	if ( players[clientnum]->entity->effectShapeshift != NOTHING )
	{
		playerRace = static_cast<Monster>(players[clientnum]->entity->effectShapeshift);
	}
	else if ( players[clientnum]->entity->effectPolymorph != NOTHING )
	{
		if ( players[clientnum]->entity->effectPolymorph > NUMMONSTERS )
		{
			playerRace = HUMAN;
			playerAppearance = players[clientnum]->entity->effectPolymorph - 100;
		}
		else
		{
			playerRace = static_cast<Monster>(players[clientnum]->entity->effectPolymorph);
		}
	}

	bool noGloves = false;
	bool hideWeapon = false;
	if (stats[clientnum]->gloves == nullptr
		|| playerRace == SPIDER
		|| playerRace == RAT
		|| playerRace == CREATURE_IMP
		|| playerRace == TROLL )
	{
		noGloves = true;
		hideWeapon = true;
	}
	else
	{
		if ( stats[clientnum]->gloves->type == GLOVES || stats[clientnum]->gloves->type == GLOVES_DEXTERITY )
		{
			my->sprite = 637;
		}
		else if ( stats[clientnum]->gloves->type == BRACERS || stats[clientnum]->gloves->type == BRACERS_CONSTITUTION )
		{
			my->sprite = 638;
		}
		else if ( stats[clientnum]->gloves->type == GAUNTLETS || stats[clientnum]->gloves->type == GAUNTLETS_STRENGTH )
		{
			my->sprite = 639;
		}
		else if ( stats[clientnum]->gloves->type == BRASS_KNUCKLES )
		{
			my->sprite = 640;
		}
		else if ( stats[clientnum]->gloves->type == IRON_KNUCKLES )
		{
			my->sprite = 641;
		}
		else if ( stats[clientnum]->gloves->type == SPIKED_GAUNTLETS )
		{
			my->sprite = 642;
		}
		else if ( stats[clientnum]->gloves->type == CRYSTAL_GLOVES )
		{
			my->sprite = 591;
		}
		else if ( stats[clientnum]->gloves->type == ARTIFACT_GLOVES )
		{
			my->sprite = 590;
		}
		else if ( stats[clientnum]->gloves->type == SUEDE_GLOVES )
		{
			my->sprite = 802;
		}
		if ( stats[clientnum]->weapon == nullptr )
		{
			my->scalex = 0.5f;
			my->scaley = 0.5f;
			my->scalez = 0.5f;
			my->z -= 0.75;
			//my->x += 0.5 * cos(parent->yaw);
			//my->y += 0.5 * sin(parent->yaw);
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
				if ( stats[clientnum]->sex == FEMALE )
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
				my->sprite = 853;
				break;
			case CREATURE_IMP:
				my->sprite = 857;
				break;
			case RAT:
				my->sprite = 859;
				break;
			default:
				my->sprite = 634;
				break;
		}

		if ( stats[clientnum]->weapon == nullptr || hideWeapon )
		{
			my->scalex = 0.5f;
			my->scaley = 0.5f;
			my->scalez = 0.5f;
			my->z -= 0.75;
			//my->x += 0.5 * cos(parent->yaw);
			//my->y += 0.5 * sin(parent->yaw);
		}
	}

	// rotation
	//my->yaw = atan2( my->y-camera.y*16, my->x-camera.x*16 );
	//my->fskill[0] = sqrt( pow(my->x-camera.x*16,2) + pow(my->y-camera.y*16,2) );
	//my->pitch = atan2( my->z-camera.z*.5, my->fskill[0] );
	//messagePlayer(0, "my y: %f, my z: %f", my->y, my->z);
	my->focalx = 0;
	my->focaly = 0;
	my->focalz = -1.5;
	if ( playerRace == RAT )
	{
		my->pitch = 0.f;
		my->yaw = parent->yaw + players[clientnum]->entity->fskill[10]; // PLAYER_SIDEBOB
		my->scalex = 1.f;
		my->scaley = 1.f;
		my->scalez = 1.f;
		my->x += -4.f;
		my->y += -3.f;
		my->z += 5.5;
	}
	else if ( playerRace == SPIDER )
	{
		my->pitch = parent->pitch;
		my->yaw = parent->yaw + players[clientnum]->entity->fskill[10];
		my->scalex = 1.f;
		my->scaley = 1.f;
		my->scalez = 1.f;
		my->x += 1;
		my->y += 1;
		my->z += 2;
		my->focalz = -1;
	}
	else
	{
		my->yaw = -2 * PI / 32;
		my->pitch = -17 * PI / 32;
	}
}

bool bowFire = false;
bool bowIsBeingDrawn = false;
Uint32 bowStartDrawingTick = 0;
Uint32 bowDrawBaseTicks = 50;
#ifdef USE_FMOD
	FMOD_CHANNEL* bowDrawingSoundChannel = NULL;
	FMOD_BOOL bowDrawingSoundPlaying = 0;
#elif defined USE_OPENAL
	OPENAL_SOUND* bowDrawingSoundChannel = NULL;
	ALboolean bowDrawingSoundPlaying = 0;
#endif

#define HUDWEAPON_CHOP my->skill[0]
#define HUDWEAPON_INIT my->skill[1]
#define HUDWEAPON_CHARGE my->skill[3]
#define HUDWEAPON_OVERCHARGE my->skill[4]
#define HUDWEAPON_WHIP_ANGLE my->skill[5]
#define HUDWEAPON_HIDEWEAPON my->skill[6]
#define HUDWEAPON_SHOOTING_RANGED_WEAPON my->skill[7]
#define HUDWEAPON_RANGED_QUIVER_RELOAD my->skill[8]
#define HUDWEAPON_MOVEX my->fskill[0]
#define HUDWEAPON_MOVEY my->fskill[1]
#define HUDWEAPON_MOVEZ my->fskill[2]
#define HUDWEAPON_YAW my->fskill[3]
#define HUDWEAPON_PITCH my->fskill[4]
#define HUDWEAPON_ROLL my->fskill[5]
#define HUDWEAPON_OLDVIBRATEX my->fskill[6]
#define HUDWEAPON_OLDVIBRATEY my->fskill[7]
#define HUDWEAPON_OLDVIBRATEZ my->fskill[8]

Uint32 hudweaponuid = 0;
void actHudWeapon(Entity* my)
{
	double result = 0;
	ItemType type;
	bool wearingring = false;
	Entity* entity;
	Entity* parent = hudarm;

	// isn't active during intro/menu sequence
	if ( intro == true )
	{
		my->flags[INVISIBLE] = true;
		return;
	}

	if ( multiplayer == CLIENT )
	{
		if ( stats[clientnum]->HP <= 0 )
		{
			my->flags[INVISIBLE] = true;
			return;
		}
	}

	// initialize
	if ( !HUDWEAPON_INIT )
	{
		HUDWEAPON_INIT = 1;
		hudweapon = my;
		hudweaponuid = my->getUID();
		entity = newEntity(109, 1, map.entities, nullptr); // malearmright.vox
		entity->focalz = -1.5;
		entity->parent = my->getUID();
		my->parent = entity->getUID(); // just an easy way to refer to eachother, doesn't mean much
		hudarm = entity;
		parent = hudarm;
		entity->behavior = &actHudArm;
		entity->flags[OVERDRAW] = true;
		entity->flags[PASSABLE] = true;
		entity->flags[NOUPDATE] = true;
	}

	if ( players[clientnum] == nullptr || players[clientnum]->entity == nullptr 
		|| (multiplayer != CLIENT && players[clientnum]->entity && players[clientnum]->entity->playerCreatedDeathCam != 0) )
	{
		hudweapon = nullptr; //PLAYER DED. NULLIFY THIS.
		list_RemoveNode(my->mynode);
		return;
	}

	// reduce throwGimpTimer (allows player to throw items again)
	if ( throwGimpTimer > 0 )
	{
		--throwGimpTimer;
	}
	if ( pickaxeGimpTimer > 0 )
	{
		//messagePlayer(clientnum, "%d", pickaxeGimpTimer);
		--pickaxeGimpTimer;
	}
	if ( swapWeaponGimpTimer > 0 )
	{
		--swapWeaponGimpTimer;
	}

	// check levitating value
	bool levitating = isLevitating(stats[clientnum]);

	// water walking boots
	bool waterwalkingboots = false;
	if (stats[clientnum]->shoes != nullptr)
		if ( stats[clientnum]->shoes->type == IRON_BOOTS_WATERWALKING )
		{
			waterwalkingboots = true;
		}

	// swimming
	if (players[clientnum] && players[clientnum]->entity)
	{
		if (!levitating && !waterwalkingboots && !skillCapstoneUnlocked(clientnum, PRO_SWIMMING) )
		{
			int x = std::min<unsigned>(std::max<int>(0, floor(players[clientnum]->entity->x / 16)), map.width - 1);
			int y = std::min<unsigned>(std::max<int>(0, floor(players[clientnum]->entity->y / 16)), map.height - 1);
			if ( swimmingtiles[map.tiles[y * MAPLAYERS + x * MAPLAYERS * map.height]] || lavatiles[map.tiles[y * MAPLAYERS + x * MAPLAYERS * map.height]] )
			{
				my->flags[INVISIBLE] = true;
				if (parent)
				{
					parent->flags[INVISIBLE] = true;
				}
				return;
			}
		}
	}

	Monster playerRace = players[clientnum]->entity->getMonsterFromPlayerRace(stats[clientnum]->playerRace);
	int playerAppearance = stats[clientnum]->appearance;
	if ( players[clientnum]->entity->effectShapeshift != NOTHING )
	{
		playerRace = static_cast<Monster>(players[clientnum]->entity->effectShapeshift);
	}
	else if ( players[clientnum]->entity->effectPolymorph != NOTHING )
	{
		if ( players[clientnum]->entity->effectPolymorph > NUMMONSTERS )
		{
			playerRace = HUMAN;
			playerAppearance = players[clientnum]->entity->effectPolymorph - 100;
		}
		else
		{
			playerRace = static_cast<Monster>(players[clientnum]->entity->effectPolymorph);
		}
	}

	bool hideWeapon = false;
	if ( playerRace == SPIDER
		|| playerRace == RAT
		|| playerRace == CREATURE_IMP
		|| playerRace == TROLL )
	{
		hideWeapon = true;
		if ( playerRace == CREATURE_IMP && stats[clientnum]->weapon && itemCategory(stats[clientnum]->weapon) == MAGICSTAFF )
		{
			hideWeapon = false;
		}
	}

	HUDWEAPON_HIDEWEAPON = hideWeapon;
	HUDWEAPON_SHOOTING_RANGED_WEAPON = 0;

	bool rangedweapon = false;
	if ( stats[clientnum]->weapon && !hideWeapon )
	{
		if ( isRangedWeapon(*stats[clientnum]->weapon) )
		{
			rangedweapon = true;
		}
	}

	// select model
	if ( stats[clientnum]->ring != nullptr )
	{
		if ( stats[clientnum]->ring->type == RING_INVISIBILITY )
		{
			wearingring = true;
		}
	}
	if ( stats[clientnum]->cloak != nullptr )
	{
		if ( stats[clientnum]->cloak->type == CLOAK_INVISIBILITY )
		{
			wearingring = true;
		}
	}
	if ( players[clientnum]->entity->skill[3] == 1 || players[clientnum]->entity->isInvisible() )   // debug cam or player invisible
	{
		my->flags[INVISIBLE] = true;
		if (parent != nullptr)
		{
			parent->flags[INVISIBLE] = true;
		}
	}
	else
	{
		if ( stats[clientnum]->weapon == nullptr || hideWeapon )
		{
			my->flags[INVISIBLE] = true;
			if (parent != nullptr)
			{
				parent->flags[INVISIBLE] = false;
			}
		}
		else
		{
			if ( stats[clientnum]->weapon )
			{
				if ( stats[clientnum]->weapon->type == TOOL_WHIP )
				{
					my->scalex = 0.6;
					my->scaley = 0.6;
					my->scalez = 0.6;
				}
				else if ( stats[clientnum]->weapon->type == TOOL_DECOY 
					|| stats[clientnum]->weapon->type == TOOL_DUMMYBOT )
				{
					my->scalex = 0.8;
					my->scaley = 0.8;
					my->scalez = 0.8;
				}
				else if ( stats[clientnum]->weapon->type == TOOL_SENTRYBOT
					|| stats[clientnum]->weapon->type == TOOL_SPELLBOT )
				{
					my->scalex = 0.7;
					my->scaley = 0.7;
					my->scalez = 0.7;
				}
				else if ( stats[clientnum]->weapon->type >= TOOL_BOMB && stats[clientnum]->weapon->type <= TOOL_TELEPORT_BOMB )
				{
					my->scalex = 1.f;
					my->scaley = 1.f;
					my->scalez = 1.f;
				}
				else if ( itemModelFirstperson(stats[clientnum]->weapon) != itemModel(stats[clientnum]->weapon) )
				{
					my->scalex = 0.5f;
					my->scaley = 0.5f;
					my->scalez = 0.5f;
				}
				else
				{
					my->scalex = 1.f;
					my->scaley = 1.f;
					my->scalez = 1.f;
				}
			}
			my->sprite = itemModelFirstperson(stats[clientnum]->weapon);

			if ( bowIsBeingDrawn )
			{
				HUDWEAPON_SHOOTING_RANGED_WEAPON = 1;
				if ( (my->ticks - bowStartDrawingTick) >= bowDrawBaseTicks / 4 )
				{
					if ( rangedweapon )
					{
						if ( stats[clientnum]->weapon && stats[clientnum]->weapon->type != CROSSBOW )
						{
							my->sprite++;
						}
						HUDWEAPON_SHOOTING_RANGED_WEAPON = 2;
					}
				}
			}
			else if ( stats[clientnum]->weapon && stats[clientnum]->weapon->type == CROSSBOW )
			{
				HUDWEAPON_SHOOTING_RANGED_WEAPON = 1;
				if ( HUDWEAPON_CHOP != 0 )
				{
					if ( HUDWEAPON_CHOP == 16 )
					{
						my->sprite = 975;
					}
					HUDWEAPON_SHOOTING_RANGED_WEAPON = 2;
				}
			}

			if ( itemCategory(stats[clientnum]->weapon) == SPELLBOOK )
			{
				my->flags[INVISIBLE] = true;
				if ( parent != NULL )
				{
					parent->flags[INVISIBLE] = false;
				}
			}
			else
			{
				my->flags[INVISIBLE] = false;
				if ( parent != NULL )
				{
					parent->flags[INVISIBLE] = true;
				}
			}
		}
	}

	if ( cast_animation.active || cast_animation.active_spellbook )
	{
		if ( playerRace != RAT )
		{
			my->flags[INVISIBLE] = true;
			if (parent != NULL)
			{
				parent->flags[INVISIBLE] = true;
			}
		}
	}

	bool swingweapon = false;
	if ( players[clientnum]->entity 
		&& (*inputPressed(impulses[IN_ATTACK]) || (shootmode && *inputPressed(joyimpulses[INJOY_GAME_ATTACK]))) 
		&& shootmode 
		&& !gamePaused
		&& players[clientnum]->entity->isMobile() 
		&& !(*inputPressed(impulses[IN_DEFEND]) && stats[clientnum]->defending || (shootmode && *inputPressed(joyimpulses[INJOY_GAME_DEFEND]) && stats[clientnum]->defending))
		&& HUDWEAPON_OVERCHARGE < MAXCHARGE )
	{
		swingweapon = true;
	}
	else if ( (*inputPressed(impulses[IN_ATTACK]) || (shootmode && *inputPressed(joyimpulses[INJOY_GAME_ATTACK]))) &&
		(*inputPressed(impulses[IN_DEFEND]) && stats[clientnum]->defending || (shootmode && *inputPressed(joyimpulses[INJOY_GAME_DEFEND]) && stats[clientnum]->defending)) )
	{
		if ( stats[clientnum]->shield && stats[clientnum]->shield->type == TOOL_TINKERING_KIT )
		{
			if ( !GenericGUI.isGUIOpen() )
			{
				*inputPressed(impulses[IN_ATTACK]) = 0;
				*inputPressed(joyimpulses[INJOY_GAME_ATTACK]) = 0;
				GenericGUI.openGUI(GUI_TYPE_TINKERING, stats[clientnum]->shield);
				swapWeaponGimpTimer = 20;
				return;
			}
		}
	}

	bool castStrikeAnimation = (players[clientnum]->entity->skill[9] == MONSTER_POSE_SPECIAL_WINDUP1);

	// weapon switch animation
	if ( weaponSwitch )
	{
		weaponSwitch = false;
		if ( !hideWeapon && stats[clientnum]->weapon && stats[clientnum]->weapon->type == CROSSBOW )
		{
			swingweapon = false;
			HUDWEAPON_CHARGE = 0;
			HUDWEAPON_OVERCHARGE = 0;
			HUDWEAPON_CHOP = 0;
			throwGimpTimer = std::max(throwGimpTimer, 20);
			
			if ( rangedWeaponUseQuiverOnAttack(stats[clientnum]) )
			{
				HUDWEAPON_RANGED_QUIVER_RELOAD = 4;
				if ( swingweapon )
				{
					swapWeaponGimpTimer = 20; // gimp timer for quivers and ranged weapons.
				}
				else
				{
					// let go of attack button, if the animation is the post-attack portion, then quickly fade the timer.
					if ( swapWeaponGimpTimer > 5 )
					{
						swapWeaponGimpTimer = 5;
					}
				}
				swingweapon = false;
			}
		}

		if ( !HUDWEAPON_CHOP && !hideWeapon )
		{
			HUDWEAPON_MOVEZ = 2;
			HUDWEAPON_MOVEX = -.5;
			HUDWEAPON_ROLL = -PI / 2;
		}
	}

	// all items (e.g weapons) have swap equipment gimp timer in multiplayer.
	if ( HUDWEAPON_CHOP > 0 && swingweapon )
	{
		swapWeaponGimpTimer = 20;
	}
	else if ( HUDWEAPON_CHOP != 0 && HUDWEAPON_CHOP != 1 && HUDWEAPON_CHOP != 4 && HUDWEAPON_CHOP != 7 && !swingweapon )
	{
		// let go of attack button, if the animation is the post-attack portion, then quickly fade the timer.
		if ( swapWeaponGimpTimer > 5 )
		{
			swapWeaponGimpTimer = 5;
		}
	}

	if ( HUDWEAPON_RANGED_QUIVER_RELOAD == 4 )
	{
		swapWeaponGimpTimer = 20;
	}

	Uint32 bowFireRate = bowDrawBaseTicks;
	bool shakeRangedWeapon = false;
	bool cancelRangedAttack = false;
	if ( rangedweapon && stats[clientnum]->weapon 
		&& stats[clientnum]->weapon->type != CROSSBOW
		&& stats[clientnum]->weapon->type != HEAVY_CROSSBOW
		&& !hideWeapon )
	{
		bowFireRate = bowDrawBaseTicks * (rangedAttackGetSpeedModifier(stats[clientnum]));

		if ( swingweapon && HUDWEAPON_CHOP != 0 )
		{
			swingweapon = false;
			HUDWEAPON_CHARGE = 0;
			HUDWEAPON_OVERCHARGE = 0;
			HUDWEAPON_CHOP = 0;
			bowIsBeingDrawn = false;
		}
		else if ( rangedWeaponUseQuiverOnAttack(stats[clientnum]) )
		{
			if ( swingweapon )
			{
				swapWeaponGimpTimer = 20; // gimp timer for quivers and ranged weapons.
			}
			else
			{
				// let go of attack button, if the animation is the post-attack portion, then quickly fade the timer.
				if ( swapWeaponGimpTimer > 5 )
				{
					swapWeaponGimpTimer = 5;
				}
			}
		}
	}
	// check bow drawing attack and if defending/not firing cancel the SFX.
	if ( rangedweapon && bowIsBeingDrawn && !stats[clientnum]->defending && !hideWeapon )
	{
		if ( (my->ticks - bowStartDrawingTick) < bowFireRate )
		{
			/*if ( (my->ticks - bowStartDrawingTick) > bowFireRate * 0.75 )
			{
				shakeRangedWeapon = true;
			}*/
			//messagePlayer(clientnum, "ticks: %d", bowFireRate);
			bowFire = false;
		}
		else
		{
			bowIsBeingDrawn = false;
			bowFire = true; // ready to fire!
		}
#ifdef SOUND
#ifdef USE_OPENAL
		if ( bowDrawingSoundChannel )
		{
			OPENAL_Channel_IsPlaying(bowDrawingSoundChannel, &bowDrawingSoundPlaying);
		}
#else
		if ( bowDrawingSoundChannel )
		{
			FMOD_Channel_IsPlaying(bowDrawingSoundChannel, &bowDrawingSoundPlaying);
		}
#endif
#endif // SOUND
	}
	else
	{
		bowFire = false;
		bowIsBeingDrawn = false;
#ifdef SOUND
		if ( bowDrawingSoundPlaying && bowDrawingSoundChannel )
		{
#ifdef USE_OPENAL
			OPENAL_Channel_Stop(bowDrawingSoundChannel);
#else
			FMOD_Channel_Stop(bowDrawingSoundChannel);
#endif
			bowDrawingSoundPlaying = 0;
			bowDrawingSoundChannel = NULL;
		}
#endif
	}

	bool whip = stats[clientnum]->weapon && stats[clientnum]->weapon->type == TOOL_WHIP;
	bool thrownWeapon = stats[clientnum]->weapon && (itemCategory(stats[clientnum]->weapon) == THROWN || itemCategory(stats[clientnum]->weapon) == GEM);

	//messagePlayer(clientnum, "chop: %d", HUDWEAPON_CHOP);

	// main animation
	if ( HUDWEAPON_CHOP == 0 )
	{
		if ( HUDWEAPON_RANGED_QUIVER_RELOAD != 4 )
		{
			HUDWEAPON_RANGED_QUIVER_RELOAD = 0;
		}
		bool ignoreAttack = false;
		if ( swingweapon && throwGimpTimer > 0 && stats[clientnum]->weapon &&
			( stats[clientnum]->weapon->type == CROSSBOW
				|| itemCategory(stats[clientnum]->weapon) == POTION
				|| itemCategory(stats[clientnum]->weapon) == GEM
				|| itemCategory(stats[clientnum]->weapon) == THROWN)
			)
		{
			ignoreAttack = true;
		}

		if ( !ignoreAttack && (swingweapon || castStrikeAnimation) )
		{
			if ( cast_animation.active || cast_animation.active_spellbook )
			{
				messagePlayer(clientnum, language[1301]);
				spellcastingAnimationManager_deactivate(&cast_animation);
			}
			if ( castStrikeAnimation )
			{
				HUDWEAPON_CHOP = 10; // special punch
			}
			else if ( stats[clientnum]->weapon == NULL || hideWeapon )
			{
				HUDWEAPON_CHOP = 7; // punch
			}
			else
			{
				if ( conductGameChallenges[CONDUCT_BRAWLER] || achievementBrawlerMode )
				{
					if ( itemCategory(stats[clientnum]->weapon) == WEAPON 
						|| rangedweapon 
						|| itemCategory(stats[clientnum]->weapon) == THROWN
						|| itemCategory(stats[clientnum]->weapon) == MAGICSTAFF
						|| itemCategory(stats[clientnum]->weapon) == GEM
						|| stats[clientnum]->weapon->type == TOOL_PICKAXE )
					{
						if ( achievementBrawlerMode && conductGameChallenges[CONDUCT_BRAWLER] )
						{
							messagePlayer(clientnum, language[2997]); // prevent attack.
							return;
						}
						if ( achievementBrawlerMode )
						{
							messagePlayer(clientnum, language[2998]); // notify no longer eligible for achievement but still atk.
						}
						conductGameChallenges[CONDUCT_BRAWLER] = 0;
					}
				}

				if ( itemCategory(stats[clientnum]->weapon) == WEAPON || stats[clientnum]->weapon->type == TOOL_PICKAXE )
				{
					if ( stats[clientnum]->weapon->type == TOOL_PICKAXE )
					{
						pickaxeGimpTimer = 40;
					}
					if ( stats[clientnum]->weapon->type == IRON_SPEAR || stats[clientnum]->weapon->type == ARTIFACT_SPEAR )
					{
						HUDWEAPON_CHOP = 7; // spear lunges
					}
					else if ( whip )
					{
						HUDWEAPON_CHOP = 4;
						pickaxeGimpTimer = 20;
					}
					else if ( rangedweapon )
					{
						if ( stats[clientnum]->weapon->type == SLING
							|| stats[clientnum]->weapon->type == SHORTBOW
							|| stats[clientnum]->weapon->type == ARTIFACT_BOW
							|| stats[clientnum]->weapon->type == LONGBOW
							|| stats[clientnum]->weapon->type == COMPOUND_BOW )
						{
							if ( !stats[clientnum]->defending && !throwGimpTimer )
							{
								// bows need to be drawn back
								if ( !bowIsBeingDrawn )
								{
									if ( bowFire )
									{
										bowFire = false;
										players[clientnum]->entity->attack(0, 0, nullptr);
										HUDWEAPON_MOVEX = 3;
										throwGimpTimer = TICKS_PER_SECOND / 4;

										if ( multiplayer == CLIENT )
										{
											if ( rangedWeaponUseQuiverOnAttack(stats[clientnum]) )
											{
												Item* quiver = stats[clientnum]->shield;
												quiver->count--;
												if ( quiver->count <= 0 )
												{
													if ( quiver->node )
													{
														list_RemoveNode(quiver->node);
													}
													else
													{
														free(quiver);
													}
													stats[clientnum]->shield = NULL;
												}
											}
										}
									}
									else
									{
										bowStartDrawingTick = my->ticks;
										bowIsBeingDrawn = true;
										bowDrawingSoundChannel = playSound(246, 64);
									}
								}

								if ( HUDWEAPON_MOVEX > 0 )
								{
									if ( HUDWEAPON_MOVEX > 1 )
									{
										HUDWEAPON_MOVEX = std::max<real_t>(HUDWEAPON_MOVEX - 1, 0.0);
									}
									else
									{
										HUDWEAPON_MOVEX = std::max<real_t>(HUDWEAPON_MOVEX - 0.2, 0.0);
									}
								}
								else if ( HUDWEAPON_MOVEX < 0 )
								{
									HUDWEAPON_MOVEX = std::min<real_t>(HUDWEAPON_MOVEX + 1, 0.0);
								}
								if ( HUDWEAPON_MOVEY > -1 )
								{
									HUDWEAPON_MOVEY = std::max<real_t>(HUDWEAPON_MOVEY - 1, -1.0);
								}
								else if ( HUDWEAPON_MOVEY < -1 )
								{
									HUDWEAPON_MOVEY = std::min<real_t>(HUDWEAPON_MOVEY + 1, -1.0);
								}
								if ( HUDWEAPON_MOVEZ > 0 )
								{
									HUDWEAPON_MOVEZ = std::max<real_t>(HUDWEAPON_MOVEZ - 1, 0.0);
								}
								else if ( HUDWEAPON_MOVEZ < 0 )
								{
									HUDWEAPON_MOVEZ = std::min<real_t>(HUDWEAPON_MOVEZ + 1, 0.0);
								}
								if ( HUDWEAPON_YAW > -.1 )
								{
									HUDWEAPON_YAW = std::max<real_t>(HUDWEAPON_YAW - .1, -.1);
								}
								else if ( HUDWEAPON_YAW < -.1 )
								{
									HUDWEAPON_YAW = std::min<real_t>(HUDWEAPON_YAW + .1, -.1);
								}
								if ( HUDWEAPON_PITCH > 0 )
								{
									HUDWEAPON_PITCH = std::max<real_t>(HUDWEAPON_PITCH - .1, 0.0);
								}
								else if ( HUDWEAPON_PITCH < 0 )
								{
									HUDWEAPON_PITCH = std::min<real_t>(HUDWEAPON_PITCH + .1, 0.0);
								}
								if ( HUDWEAPON_ROLL > 0 )
								{
									HUDWEAPON_ROLL = std::max<real_t>(HUDWEAPON_ROLL - .1, 0.0);
								}
								else if ( HUDWEAPON_ROLL < 0 )
								{
									HUDWEAPON_ROLL = std::min<real_t>(HUDWEAPON_ROLL + .1, 0.0);
								}
							}
						}
						else
						{
							// crossbows
							if ( throwGimpTimer == 0 )
							{
								players[clientnum]->entity->attack(0, 0, nullptr);
								HUDWEAPON_MOVEX = -4;
								HUDWEAPON_CHOP = 3;
								// set delay before crossbow can fire again
								throwGimpTimer = 40;

								if ( rangedWeaponUseQuiverOnAttack(stats[clientnum]) )
								{
									HUDWEAPON_CHOP = 16;
									HUDWEAPON_RANGED_QUIVER_RELOAD = 1;
									throwGimpTimer = 40;
								}
							}
						}
					}
					else
					{
						HUDWEAPON_CHOP = 1;
					}
				}
				else
				{
					Item* item = stats[clientnum]->weapon;
					if (item)
					{
						if (itemCategory(item) == SPELLBOOK)
						{
							mousestatus[SDL_BUTTON_LEFT] = 0;
							players[clientnum]->entity->attack(2, 0, nullptr); // will need to add some delay to this so you can't rapid fire spells
						}
						else if ( itemIsThrowableTinkerTool(item) )
						{
							HUDWEAPON_CHOP = 13;
						}
						else if (itemCategory(item) == MAGICSTAFF)
						{
							HUDWEAPON_CHOP = 7; // magicstaffs lunge
						}
						else if ( itemCategory(item) == THROWN || itemCategory(item) == GEM )
						{
							HUDWEAPON_CHOP = 1; // thrown normal swing
						}
						else if (item->type == TOOL_LOCKPICK || item->type == TOOL_SKELETONKEY )
						{
							// keys and lockpicks
							HUDWEAPON_MOVEX = 5;
							HUDWEAPON_CHOP = 3;
							Entity* player = players[clientnum]->entity;
							bool foundBomb = false;
							if ( stats[clientnum]->weapon )
							{
								bool clickedOnGUI = false;
								int tmpmousex = omousex;
								int tmpmousey = omousey;
								omousex = xres / 2; // pretend move the mouse to the centre of screen.
								omousey = yres / 2;
								Entity* clickedOn = entityClicked(&clickedOnGUI, true); // using objects
								omousex = tmpmousex;
								omousey = tmpmousey;
								if ( clickedOn && clickedOn->behavior == &actBomb && entityDist(clickedOn, players[clientnum]->entity) < STRIKERANGE )
								{
									// found something
									stats[clientnum]->weapon->apply(clientnum, clickedOn);
									foundBomb = true;
								}
							}
							if ( !foundBomb )
							{
								lineTrace(player, player->x, player->y, player->yaw, STRIKERANGE, 0, false);
								if ( hit.entity && stats[clientnum]->weapon )
								{
									stats[clientnum]->weapon->apply(clientnum, hit.entity);
								}
								else
								{
									messagePlayer(clientnum, language[503], item->getName());
								}
							}
						}
						else if ( item->type >= ARTIFACT_ORB_BLUE && item->type <= ARTIFACT_ORB_GREEN )
						{
							HUDWEAPON_MOVEX = 5;
							HUDWEAPON_CHOP = 3;
							Entity* player = players[clientnum]->entity;
							lineTrace(player, player->x, player->y, player->yaw, STRIKERANGE, 0, false);
							if ( hit.entity && stats[clientnum]->weapon )
							{
								stats[clientnum]->weapon->apply(clientnum, hit.entity);
							}
							else
							{
								if ( statGetINT(stats[clientnum], player) <= 10 )
								{
									messagePlayer(clientnum, language[2373], item->getName());
								}
								else
								{
									messagePlayer(clientnum, language[2372], item->getName());
								}
							}
						}
						else if ( item->type == POTION_EMPTY )
						{
							if ( throwGimpTimer == 0 )
							{
								HUDWEAPON_MOVEX = 5;
								HUDWEAPON_CHOP = 3;
								Entity* player = players[clientnum]->entity;
								lineTrace(player, player->x, player->y, player->yaw, STRIKERANGE, 0, false);
								if ( hit.entity && stats[clientnum]->weapon )
								{
									stats[clientnum]->weapon->apply(clientnum, hit.entity);
								}
								else
								{
									messagePlayer(clientnum, language[3336]);
								}
								throwGimpTimer = TICKS_PER_SECOND / 2;
							}
						}
						else if ((itemCategory(item) == POTION || itemCategory(item) == GEM || itemCategory(item) == THROWN ) && !throwGimpTimer)
						{
							if ( itemCategory(item) == THROWN )
							{
								// possibility to change to be unique.
								throwGimpTimer = TICKS_PER_SECOND / 2; // limits how often you can throw objects
							}
							else
							{
								throwGimpTimer = TICKS_PER_SECOND / 2; // limits how often you can throw objects
							}
							HUDWEAPON_MOVEZ = 3;
							HUDWEAPON_CHOP = 3;
							players[clientnum]->entity->attack(1, 0, nullptr);
							if (multiplayer == CLIENT)
							{
								item->count--;
								if (item->count <= 0)
								{
									if (item->node)
									{
										list_RemoveNode(item->node);
									}
									else
									{
										free(item);
									}
									stats[clientnum]->weapon = NULL;
								}
							}
						}
					}
				}
			}
		}
		else
		{
			if ( !stats[clientnum]->defending )
			{
				if ( stats[clientnum]->weapon || hideWeapon )
				{
					if ( !hideWeapon &&
						(stats[clientnum]->weapon->type == SLING
							|| stats[clientnum]->weapon->type == SHORTBOW
							|| stats[clientnum]->weapon->type == ARTIFACT_BOW
							|| stats[clientnum]->weapon->type == LONGBOW
							|| stats[clientnum]->weapon->type == COMPOUND_BOW) )
					{
						// not drawing bow anymore, reset.
						bowIsBeingDrawn = false;
#ifdef SOUND
						if ( bowDrawingSoundPlaying && bowDrawingSoundChannel )
						{
#ifdef USE_OPENAL
							OPENAL_Channel_Stop(bowDrawingSoundChannel);
#else
							FMOD_Channel_Stop(bowDrawingSoundChannel);
#endif
							bowDrawingSoundPlaying = 0;
							bowDrawingSoundChannel = NULL;
						}
#endif

						if ( HUDWEAPON_MOVEX > 0 )
						{
							HUDWEAPON_MOVEX = std::max<real_t>(HUDWEAPON_MOVEX - 1, 0.0);
						}
						else if ( HUDWEAPON_MOVEX < 0 )
						{
							HUDWEAPON_MOVEX = std::min<real_t>(HUDWEAPON_MOVEX + 1, 0.0);
						}
						if ( HUDWEAPON_MOVEY > 1 )
						{
							HUDWEAPON_MOVEY = std::max<real_t>(HUDWEAPON_MOVEY - 1, 1.0);
						}
						else if ( HUDWEAPON_MOVEY < 1 )
						{
							HUDWEAPON_MOVEY = std::min<real_t>(HUDWEAPON_MOVEY + 1, 1.0);
						}
						if ( HUDWEAPON_MOVEZ > 0 )
						{
							HUDWEAPON_MOVEZ = std::max<real_t>(HUDWEAPON_MOVEZ - 1, 0.0);
						}
						else if ( HUDWEAPON_MOVEZ < 0 )
						{
							HUDWEAPON_MOVEZ = std::min<real_t>(HUDWEAPON_MOVEZ + 1, 0.0);
						}
						if ( HUDWEAPON_YAW > -.1 )
						{
							HUDWEAPON_YAW = std::max<real_t>(HUDWEAPON_YAW - .1, -.1);
						}
						else if ( HUDWEAPON_YAW < -.1 )
						{
							HUDWEAPON_YAW = std::min<real_t>(HUDWEAPON_YAW + .1, -.1);
						}
						if ( HUDWEAPON_PITCH > 0 )
						{
							HUDWEAPON_PITCH = std::max<real_t>(HUDWEAPON_PITCH - .1, 0.0);
						}
						else if ( HUDWEAPON_PITCH < 0 )
						{
							HUDWEAPON_PITCH = std::min<real_t>(HUDWEAPON_PITCH + .1, 0.0);
						}
						if ( HUDWEAPON_ROLL > -PI / 3 )
						{
							HUDWEAPON_ROLL = std::max<real_t>(HUDWEAPON_ROLL - .1, -PI / 3);
						}
						else if ( HUDWEAPON_ROLL < -PI / 3 )
						{
							HUDWEAPON_ROLL = std::min<real_t>(HUDWEAPON_ROLL + .1, -PI / 3);
						}
					}
					else
					{
						if ( HUDWEAPON_MOVEX > 0 )
						{
							HUDWEAPON_MOVEX = std::max<real_t>(HUDWEAPON_MOVEX - 1, 0.0);
						}
						else if ( HUDWEAPON_MOVEX < 0 )
						{
							HUDWEAPON_MOVEX = std::min<real_t>(HUDWEAPON_MOVEX + 1, 0.0);
						}
						if ( HUDWEAPON_MOVEY > 0 )
						{
							HUDWEAPON_MOVEY = std::max<real_t>(HUDWEAPON_MOVEY - 1, 0.0);
						}
						else if ( HUDWEAPON_MOVEY < 0 )
						{
							HUDWEAPON_MOVEY = std::min<real_t>(HUDWEAPON_MOVEY + 1, 0.0);
						}
						if ( HUDWEAPON_MOVEZ > 0 )
						{
							HUDWEAPON_MOVEZ = std::max<real_t>(HUDWEAPON_MOVEZ - 1, 0.0);
						}
						else if ( HUDWEAPON_MOVEZ < 0 )
						{
							HUDWEAPON_MOVEZ = std::min<real_t>(HUDWEAPON_MOVEZ + 1, 0.0);
						}
						if ( HUDWEAPON_YAW > -.1 )
						{
							HUDWEAPON_YAW = std::max<real_t>(HUDWEAPON_YAW - .1, -.1);
						}
						else if ( HUDWEAPON_YAW < -.1 )
						{
							HUDWEAPON_YAW = std::min<real_t>(HUDWEAPON_YAW + .1, -.1);
						}
						if ( HUDWEAPON_PITCH > 0 )
						{
							HUDWEAPON_PITCH = std::max<real_t>(HUDWEAPON_PITCH - .1, 0.0);
						}
						else if ( HUDWEAPON_PITCH < 0 )
						{
							HUDWEAPON_PITCH = std::min<real_t>(HUDWEAPON_PITCH + .1, 0.0);
						}
						if ( HUDWEAPON_ROLL > 0 )
						{
							HUDWEAPON_ROLL = std::max<real_t>(HUDWEAPON_ROLL - .1, 0.0);
						}
						else if ( HUDWEAPON_ROLL < 0 )
						{
							HUDWEAPON_ROLL = std::min<real_t>(HUDWEAPON_ROLL + .1, 0.0);
						}
					}
				}
				else
				{
					// fix for fists not resetting position after blocking.
					if ( HUDWEAPON_MOVEY > 0 )
					{
						HUDWEAPON_MOVEY = std::max<real_t>(HUDWEAPON_MOVEY - 1, 0.0);
					}
					else if ( HUDWEAPON_MOVEY < 0 )
					{
						HUDWEAPON_MOVEY = std::min<real_t>(HUDWEAPON_MOVEY + 1, 0.0);
					}
					if ( HUDWEAPON_MOVEZ > 0 )
					{
						HUDWEAPON_MOVEZ = std::max<real_t>(HUDWEAPON_MOVEZ - 1, 0.0);
					}
					else if ( HUDWEAPON_MOVEZ < 0 )
					{
						HUDWEAPON_MOVEZ = std::min<real_t>(HUDWEAPON_MOVEZ + 1, 0.0);
					}
				}
			}
			else
			{
				if ( HUDWEAPON_MOVEX > 0 )
				{
					HUDWEAPON_MOVEX = std::max<real_t>(HUDWEAPON_MOVEX - 1, 0.0);
				}
				else if ( HUDWEAPON_MOVEX < 0 )
				{
					HUDWEAPON_MOVEX = std::min<real_t>(HUDWEAPON_MOVEX + 1, 0.0);
				}
				if ( HUDWEAPON_MOVEY > 1 )
				{
					HUDWEAPON_MOVEY = std::max<real_t>(HUDWEAPON_MOVEY - 1, 1.0);
				}
				else if ( HUDWEAPON_MOVEY < 1 )
				{
					HUDWEAPON_MOVEY = std::min<real_t>(HUDWEAPON_MOVEY + 1, 1.0);
				}
				if ( HUDWEAPON_MOVEZ > 1 )
				{
					HUDWEAPON_MOVEZ = std::max<real_t>(HUDWEAPON_MOVEZ - 1, 1.0);
				}
				else if ( HUDWEAPON_MOVEZ < 1 )
				{
					HUDWEAPON_MOVEZ = std::min<real_t>(HUDWEAPON_MOVEZ + 1, 1.0);
				}
				if ( HUDWEAPON_YAW > .1 )
				{
					HUDWEAPON_YAW = std::max<real_t>(HUDWEAPON_YAW - .1, .1);
				}
				else if ( HUDWEAPON_YAW < .1 )
				{
					HUDWEAPON_YAW = std::min<real_t>(HUDWEAPON_YAW + .1, .1);
				}
				if ( HUDWEAPON_PITCH > PI / 6 )
				{
					HUDWEAPON_PITCH = std::max<real_t>(HUDWEAPON_PITCH - .1, PI / 6);
				}
				else if ( HUDWEAPON_PITCH < PI / 6 )
				{
					HUDWEAPON_PITCH = std::min<real_t>(HUDWEAPON_PITCH + .1, PI / 6);
				}
				if ( HUDWEAPON_ROLL > PI / 6 )
				{
					HUDWEAPON_ROLL = std::max<real_t>(HUDWEAPON_ROLL - .1, PI / 6);
				}
				else if ( HUDWEAPON_ROLL < PI / 6 )
				{
					HUDWEAPON_ROLL = std::min<real_t>(HUDWEAPON_ROLL + .1, PI / 6);
				}
			}
		}
	}
	else if ( HUDWEAPON_CHOP == 1 )     // prepare for first swing
	{
		HUDWEAPON_YAW -= .25;
		if ( HUDWEAPON_YAW < 0 )
		{
			HUDWEAPON_YAW = 0;
		}
		HUDWEAPON_PITCH -= .1;
		if ( HUDWEAPON_PITCH < -PI / 4)
		{
			result = -PI / 4;
			HUDWEAPON_PITCH = result;
		}
		HUDWEAPON_ROLL += .25;
		if ( HUDWEAPON_ROLL > 0 )
		{
			HUDWEAPON_ROLL = 0;
		}
		int targetY = -2;
		if ( thrownWeapon )
		{
			targetY = -1;
			HUDWEAPON_MOVEY -= .25;
			HUDWEAPON_MOVEX -= .15;
		}
		else
		{
			HUDWEAPON_MOVEY -= .45;
			HUDWEAPON_MOVEX -= .35;
		}
		if ( HUDWEAPON_MOVEX < -1 )
		{
			HUDWEAPON_MOVEX = -1;
		}
		if ( HUDWEAPON_MOVEY < targetY )
		{
			HUDWEAPON_MOVEY = targetY;
		}
		int targetZ = -6;
		if ( whip )
		{
			targetZ = -6;
			HUDWEAPON_MOVEZ -= .32;
			if ( HUDWEAPON_MOVEY > 2 )
			{
				HUDWEAPON_MOVEY -= .45; // returning from side swing, y offset is larger than normal so assist here.
			}
		}
		else if ( thrownWeapon )
		{
			targetZ = -3;
			HUDWEAPON_MOVEZ -= .35;
		}
		else
		{
			HUDWEAPON_MOVEZ -= .65;
		}
		if ( HUDWEAPON_MOVEZ < targetZ )
		{
			HUDWEAPON_MOVEZ = targetZ;
			if ( HUDWEAPON_PITCH == result && HUDWEAPON_ROLL == 0 && HUDWEAPON_YAW == 0 && HUDWEAPON_MOVEX == -1 && HUDWEAPON_MOVEY == targetY )
			{
				if ( !swingweapon )
				{
					HUDWEAPON_CHOP++;
					players[clientnum]->entity->attack(1, HUDWEAPON_CHARGE, nullptr);
					if ( stats[clientnum]->weapon
						&& stats[clientnum]->weapon->type == CROSSBOW )
					{
						throwGimpTimer = 40; // fix for swapping weapon to crossbow while charging.
					}
					if ( (stats[clientnum]->weapon
						&& stats[clientnum]->weapon->type == TOOL_PICKAXE) || whip )
					{
						if ( pickaxeGimpTimer < 20 )
						{
							pickaxeGimpTimer = 20; // fix for swapping weapon from pickaxe causing issues.
						}
					}

					if ( thrownWeapon && multiplayer == CLIENT )
					{
						Item* item = stats[clientnum]->weapon;
						if ( item )
						{
							item->count--;
							if ( item->count <= 0 )
							{
								if ( item->node )
								{
									list_RemoveNode(item->node);
								}
								else
								{
									free(item);
								}
								stats[clientnum]->weapon = NULL;
							}
						}
					}

					HUDWEAPON_CHARGE = 0;
					HUDWEAPON_OVERCHARGE = 0;
					if (players[clientnum]->entity->skill[3] == 0)   // debug cam OFF
					{
						camera_shakey += 6;
					}
				}
				else
				{
					HUDWEAPON_CHARGE = std::min<real_t>(HUDWEAPON_CHARGE + 1, MAXCHARGE);
				}
			}
		}
	}
	else if ( HUDWEAPON_CHOP == 2 )     // first swing
	{
		if ( whip )
		{
			real_t animationMult = 0.8;
			HUDWEAPON_MOVEX += 1 * animationMult;
			if ( HUDWEAPON_MOVEX > 4 )
			{
				HUDWEAPON_MOVEX = 4;
			}
			HUDWEAPON_MOVEY += .45 * animationMult;
			if ( HUDWEAPON_MOVEY > 0 )
			{
				HUDWEAPON_MOVEY = 0;
			}
			HUDWEAPON_PITCH += .5 * animationMult;
			if ( HUDWEAPON_PITCH >= 3 * PI / 4 )
			{
				HUDWEAPON_PITCH = 3 * PI / 4;
			}
			HUDWEAPON_MOVEZ += 1 * animationMult;
			if ( HUDWEAPON_MOVEZ > 4 )
			{
				HUDWEAPON_MOVEZ = 4;
			}
			if ( HUDWEAPON_PITCH >= PI / 8 )
			{
				my->sprite = items[TOOL_WHIP].fpindex + 1;
			}
			if ( HUDWEAPON_YAW > -.1 )
			{
				HUDWEAPON_YAW = std::max<real_t>(HUDWEAPON_YAW - .1, -.1);
			}
			else if ( HUDWEAPON_YAW < -.1 )
			{
				HUDWEAPON_YAW = std::min<real_t>(HUDWEAPON_YAW + .1, -.1);
			}
			if ( HUDWEAPON_MOVEZ == 4 && HUDWEAPON_PITCH == 3 * PI / 4 )
			{
				HUDWEAPON_CHOP++;
			}
		}
		else
		{
			HUDWEAPON_PITCH += .75;
			if ( HUDWEAPON_PITCH >= (PI * 3) / 4 )
			{
				HUDWEAPON_PITCH = (PI * 3) / 4;
			}
			HUDWEAPON_MOVEX += 1;
			if ( HUDWEAPON_MOVEX > 4 )
			{
				HUDWEAPON_MOVEX = 4;
			}
			HUDWEAPON_MOVEZ += .8;
			if ( HUDWEAPON_MOVEZ > 0 )
			{
				HUDWEAPON_MOVEZ = 0;
				HUDWEAPON_CHOP++;
			}
		}
	}
	else if ( HUDWEAPON_CHOP == 3 )     // return from first swing
	{
		if ( swingweapon )
		{
			// another swing...
			Item* item = stats[clientnum]->weapon;
			if ( item && !hideWeapon )
			{
				if ( !rangedweapon 
					&& item->type != TOOL_SKELETONKEY 
					&& item->type != TOOL_LOCKPICK 
					&& itemCategory(item) != POTION 
					&& itemCategory(item) != GEM 
					&& !(item->type >= ARTIFACT_ORB_BLUE && item->type <= ARTIFACT_ORB_GREEN)
					&& !(itemIsThrowableTinkerTool(item))
					&& item->type != TOOL_WHIP )
				{
					if ( stats[clientnum]->weapon->type != TOOL_PICKAXE && itemCategory(item) != THROWN )
					{
						HUDWEAPON_CHOP = 4;
					}
					else
					{
						if ( itemCategory(item) == THROWN )
						{
							HUDWEAPON_CHOP = 0;
						}
						else if ( pickaxeGimpTimer <= 0 )
						{
							HUDWEAPON_CHOP = 1; // allow another swing of pickaxe
						}
						else
						{
							HUDWEAPON_CHOP = 0;
						}
					}
				}
			}
		}
		else
		{
			if (stats[clientnum]->weapon && !hideWeapon )
			{
				if ( stats[clientnum]->weapon->type == SLING
					|| stats[clientnum]->weapon->type == SHORTBOW
					|| stats[clientnum]->weapon->type == ARTIFACT_BOW
					|| stats[clientnum]->weapon->type == LONGBOW
					|| stats[clientnum]->weapon->type == COMPOUND_BOW )
				{
					if (bowFire)
					{
						players[clientnum]->entity->attack(0, 0, nullptr);
						HUDWEAPON_MOVEX = -2;
					}
				}
			}
		}

		bool crossbow = stats[clientnum]->weapon && stats[clientnum]->weapon->type == CROSSBOW;

		if ( stats[clientnum]->weapon && !hideWeapon )
		{
			if ( rangedweapon )
			{
				if ( HUDWEAPON_MOVEX > 0 )
				{
					HUDWEAPON_MOVEX = std::max<real_t>(HUDWEAPON_MOVEX - 1, 0.0);
				}
				else if ( HUDWEAPON_MOVEX < 0 )
				{
					HUDWEAPON_MOVEX = std::min<real_t>(HUDWEAPON_MOVEX + .1, 0.0);
				}
				if ( HUDWEAPON_MOVEY > 0 )
				{
					HUDWEAPON_MOVEY = std::max<real_t>(HUDWEAPON_MOVEY - 1, 0.0);
				}
				else if ( HUDWEAPON_MOVEY < 0 )
				{
					HUDWEAPON_MOVEY = std::min<real_t>(HUDWEAPON_MOVEY + 1, 0.0);
				}
				if ( HUDWEAPON_MOVEZ > 0 )
				{
					HUDWEAPON_MOVEZ = std::max<real_t>(HUDWEAPON_MOVEZ - 1, 0.0);
				}
				else if ( HUDWEAPON_MOVEZ < 0 )
				{
					HUDWEAPON_MOVEZ = std::min<real_t>(HUDWEAPON_MOVEZ + 1, 0.0);
				}
				if ( HUDWEAPON_YAW > -.1 )
				{
					HUDWEAPON_YAW = std::max<real_t>(HUDWEAPON_YAW - .1, -.1);
				}
				else if ( HUDWEAPON_YAW < -.1 )
				{
					HUDWEAPON_YAW = std::min<real_t>(HUDWEAPON_YAW + .1, -.1);
				}
				if ( HUDWEAPON_PITCH > 0 )
				{
					HUDWEAPON_PITCH = std::max<real_t>(HUDWEAPON_PITCH - .1, 0.0);
				}
				else if ( HUDWEAPON_PITCH < 0 )
				{
					HUDWEAPON_PITCH = std::min<real_t>(HUDWEAPON_PITCH + .1, 0.0);
				}
				if ( HUDWEAPON_ROLL > 0 )
				{
					HUDWEAPON_ROLL = std::max<real_t>(HUDWEAPON_ROLL - .1, 0.0);
				}
				else if ( HUDWEAPON_ROLL < 0 )
				{
					HUDWEAPON_ROLL = std::min<real_t>(HUDWEAPON_ROLL + .1, 0.0);
				}
			}
			else
			{
				HUDWEAPON_MOVEX -= .25;
				if ( HUDWEAPON_MOVEX < 0 )
				{
					HUDWEAPON_MOVEX = 0;
				}
			}
		}
		else
		{
			HUDWEAPON_MOVEX -= .25;
			if ( HUDWEAPON_MOVEX < 0 )
			{
				HUDWEAPON_MOVEX = 0;
			}
		}
		HUDWEAPON_PITCH -= .15;
		if ( HUDWEAPON_PITCH < 0 )
		{
			HUDWEAPON_PITCH = 0;
		}
		HUDWEAPON_MOVEY += .45;
		if ( HUDWEAPON_MOVEY > 0 )
		{
			HUDWEAPON_MOVEY = 0;
		}
		HUDWEAPON_MOVEZ -= .35;
		if ( HUDWEAPON_MOVEZ < 0 )
		{
			HUDWEAPON_MOVEZ = 0;
			if ( HUDWEAPON_PITCH == 0 && HUDWEAPON_MOVEY == 0 && HUDWEAPON_MOVEX == 0 )
			{
				HUDWEAPON_CHOP = 0;
			}
		}
	}
	else if ( HUDWEAPON_CHOP == 4 )     // prepare for second swing
	{
		int targetZ = -4;
		real_t targetRoll = -PI / 2;
		real_t rateY = .75;
		real_t rateRoll = .25;
		int targetY = -6;
		real_t targetPitch = 0.f;
		if ( whip )
		{
			HUDWEAPON_PITCH += .25;
			targetPitch = PI / 8;
			if ( HUDWEAPON_PITCH > targetPitch )
			{
				HUDWEAPON_PITCH = targetPitch;
			}
			HUDWEAPON_WHIP_ANGLE = 0;
			HUDWEAPON_YAW -= .35;
			if ( HUDWEAPON_YAW < -PI / 4 )
			{
				HUDWEAPON_YAW = -PI / 4;
			}
			rateY = .55;
			rateRoll = .35;
		}
		else
		{
			HUDWEAPON_YAW = 0;
			HUDWEAPON_PITCH -= .25;
			if ( HUDWEAPON_PITCH < targetPitch )
			{
				HUDWEAPON_PITCH = targetPitch;
			}
		}
		HUDWEAPON_MOVEX -= .35;
		if ( HUDWEAPON_MOVEX < 0 )
		{
			HUDWEAPON_MOVEX = 0;
		}
		HUDWEAPON_MOVEZ -= .75;
		if ( HUDWEAPON_MOVEZ < targetZ )
		{
			HUDWEAPON_MOVEZ = targetZ;
		}
		HUDWEAPON_MOVEY -= rateY;
		if ( HUDWEAPON_MOVEY < targetY )
		{
			HUDWEAPON_MOVEY = targetY;
		}
		HUDWEAPON_ROLL -= rateRoll;

		if (HUDWEAPON_ROLL < targetRoll )
		{
			HUDWEAPON_ROLL = targetRoll;
			if (HUDWEAPON_PITCH == targetPitch && HUDWEAPON_MOVEX == 0 && HUDWEAPON_MOVEY == targetY && HUDWEAPON_MOVEZ == targetZ)
			{
				if (!swingweapon)
				{
					HUDWEAPON_CHOP++;
					players[clientnum]->entity->attack(2, HUDWEAPON_CHARGE, nullptr);
					if ( stats[clientnum]->weapon
						&& stats[clientnum]->weapon->type == CROSSBOW )
					{
						throwGimpTimer = 40; // fix for swapping weapon to crossbow while charging.
					}
					HUDWEAPON_CHARGE = 0;
					HUDWEAPON_OVERCHARGE = 0;
					if (players[clientnum]->entity->skill[3] == 0)   // debug cam OFF
					{
						camera_shakex += .07;
					}
					if ( whip && pickaxeGimpTimer < 20 )
					{
						pickaxeGimpTimer = 20; // fix for swapping weapon from whip causing issues.
					}
				}
				else
				{
					HUDWEAPON_CHARGE = std::min<real_t>(HUDWEAPON_CHARGE + 1, MAXCHARGE);
				}
			}
		}
	}
	else if (HUDWEAPON_CHOP == 5)     // second swing
	{
		if ( whip )
		{
			if ( HUDWEAPON_WHIP_ANGLE == 0 && my->sprite == items[TOOL_WHIP].fpindex )
			{
				HUDWEAPON_WHIP_ANGLE = 1;
				HUDWEAPON_PITCH = 3 * PI / 8;	// convert from whip.vox to whip_attack.vox
				HUDWEAPON_YAW -= PI / 4;		// convert from whip.vox to whip_attack.vox
			}

			if ( HUDWEAPON_WHIP_ANGLE >= 1 )
			{
				my->sprite = items[TOOL_WHIP].fpindex + 1;
			}

			real_t animationMul = 0.8;

			HUDWEAPON_MOVEX = sin(HUDWEAPON_YAW + PI / 4) * -4;
			HUDWEAPON_MOVEY = -2 + cos(std::min(HUDWEAPON_YAW + PI / 4, 3 * PI / 4)) * -4;
			HUDWEAPON_YAW += .35 * animationMul;
			if ( HUDWEAPON_YAW > PI / 4 )
			{
				my->sprite = items[TOOL_WHIP].fpindex;
				HUDWEAPON_PITCH = PI / 8;	// convert from whip_attack.vox to whip.vox
				HUDWEAPON_YAW = PI / 2;		// convert from whip_attack.vox to whip.vox
				HUDWEAPON_CHOP++;
				HUDWEAPON_WHIP_ANGLE = 0;
				HUDWEAPON_MOVEX += 2; // trial and error got these numbers for MOVEX and MOVEY :)
				HUDWEAPON_MOVEY += 10;
			}
		}
		else
		{
			HUDWEAPON_MOVEX = sin(HUDWEAPON_YAW) * 1;
			HUDWEAPON_MOVEY = cos(HUDWEAPON_YAW) * -6;
			HUDWEAPON_YAW += .35;
			if ( HUDWEAPON_YAW > (3 * PI) / 4 )
			{
				HUDWEAPON_YAW = (3 * PI) / 4;
				HUDWEAPON_CHOP++;
			}
		}
	}
	else if (HUDWEAPON_CHOP == 6)     // return from second swing
	{
		if ( swingweapon )
		{
			// one more swing...
			if ( stats[clientnum]->weapon && !hideWeapon )
			{
				int weaponSkill = getWeaponSkill(stats[clientnum]->weapon);
				if ( weaponSkill == PRO_SWORD || stats[clientnum]->weapon->type == STEEL_HALBERD )
				{
					HUDWEAPON_CHOP = 7;  // swords + halberds can stab
				}
				else
				{
					HUDWEAPON_CHOP = 1;  // everything else can't
				}
			}
		}

		if ( whip )
		{
			real_t animationMultiplier = 0.8;
			if ( HUDWEAPON_MOVEX > 0.f )
			{
				HUDWEAPON_MOVEX = std::max<real_t>(HUDWEAPON_MOVEX - .2 * animationMultiplier, 0.f);
			}
			else if ( HUDWEAPON_MOVEX < 0.f )
			{
				HUDWEAPON_MOVEX = std::min<real_t>(HUDWEAPON_MOVEX + .2 * animationMultiplier, 0.f);
			}
			HUDWEAPON_MOVEY -= 0.9 * animationMultiplier;
			if ( HUDWEAPON_MOVEY < 0 )
			{
				HUDWEAPON_MOVEY = 0;
			}
			if ( HUDWEAPON_ROLL > -PI / 8 )
			{
				HUDWEAPON_PITCH -= .2 * animationMultiplier;
				if ( HUDWEAPON_PITCH < 0.f )
				{
					HUDWEAPON_PITCH = 0.f;
				}
			}
			HUDWEAPON_ROLL += .2 * animationMultiplier;
		}
		else
		{
			HUDWEAPON_MOVEX -= .25;
			if ( HUDWEAPON_MOVEX < 0 )
			{
				HUDWEAPON_MOVEX = 0;
			}
			HUDWEAPON_MOVEY -= .25;
			if ( HUDWEAPON_MOVEY < 0 )
			{
				HUDWEAPON_MOVEY = 0;
			}
			HUDWEAPON_ROLL += .25;
		}
		HUDWEAPON_YAW -= .25;
		if ( HUDWEAPON_YAW < -.1 )
		{
			HUDWEAPON_YAW = -.1;
		}
		HUDWEAPON_MOVEZ += .35;
		if ( HUDWEAPON_MOVEZ > 0 )
		{
			HUDWEAPON_MOVEZ = 0;
		}
		if ( HUDWEAPON_ROLL > 0 )
		{
			HUDWEAPON_ROLL = 0;
			if ( HUDWEAPON_YAW == -.1 && HUDWEAPON_MOVEZ == 0 && HUDWEAPON_MOVEY == 0 && HUDWEAPON_MOVEX == 0 )
			{
				HUDWEAPON_CHOP = 0;
			}
		}
	}
	else if ( HUDWEAPON_CHOP == 7 )     // prepare for third swing
	{
		HUDWEAPON_MOVEX -= .35;
		if ( HUDWEAPON_MOVEX < 0 )
		{
			HUDWEAPON_MOVEX = 0;
		}
		HUDWEAPON_MOVEY -= .45;
		if ( HUDWEAPON_MOVEY < -1 )
		{
			HUDWEAPON_MOVEY = -1;
		}
		HUDWEAPON_MOVEZ -= .25;
		if ( HUDWEAPON_MOVEZ < -2 )
		{
			HUDWEAPON_MOVEZ = -2;
		}
		HUDWEAPON_YAW -= .15;
		if ( HUDWEAPON_YAW < 2 * PI / 5 )
		{
			result = 2 * PI / 5;
			HUDWEAPON_YAW = result;
		}
		real_t pitchLimit = .2;
		if ( playerRace == SPIDER )
		{
			pitchLimit = -PI / 2;
			HUDWEAPON_PITCH -= .2;
		}
		else
		{
			HUDWEAPON_PITCH -= .05;
		}
		if ( HUDWEAPON_PITCH < pitchLimit )
		{
			HUDWEAPON_PITCH = pitchLimit;
		}
		HUDWEAPON_ROLL -= .15;
		if (HUDWEAPON_ROLL < -2 * PI / 5)
		{
			HUDWEAPON_ROLL = -2 * PI / 5;
			if (HUDWEAPON_PITCH == pitchLimit && HUDWEAPON_YAW == result 
				&& HUDWEAPON_MOVEX == 0 && HUDWEAPON_MOVEY == -1 && (HUDWEAPON_MOVEZ == -2))
			{
				if (!swingweapon)
				{
					HUDWEAPON_CHOP++;
					if ( stats[clientnum]->weapon && hideWeapon )
					{
						players[clientnum]->entity->attack(1, HUDWEAPON_CHARGE, nullptr);
					}
					else
					{
						players[clientnum]->entity->attack(3, HUDWEAPON_CHARGE, nullptr);
					}
					if ( stats[clientnum]->weapon
						&& stats[clientnum]->weapon->type == CROSSBOW )
					{
						throwGimpTimer = 40; // fix for swapping weapon to crossbow while charging.
					}
					HUDWEAPON_CHARGE = 0;
					HUDWEAPON_OVERCHARGE = 0;
					if (players[clientnum]->entity->skill[3] == 0)   // debug cam OFF
					{
						camera_shakex += .03;
						camera_shakey += 4;
					}
				}
				else
				{
					HUDWEAPON_CHARGE = std::min<real_t>(HUDWEAPON_CHARGE + 1, MAXCHARGE);
				}
			}
		}
	}
	else if ( HUDWEAPON_CHOP == 8 )     // third swing
	{
		if ( playerRace == SPIDER )
		{
			HUDWEAPON_MOVEX += 1;
			if ( HUDWEAPON_MOVEX > 2 )
			{
				HUDWEAPON_MOVEX = 2;
				HUDWEAPON_CHOP++;
			}
			HUDWEAPON_PITCH += .2;
			if ( HUDWEAPON_PITCH > 0 )
			{
				HUDWEAPON_PITCH = 0;
			}
		}
		else
		{
			HUDWEAPON_MOVEX += 2;
			if ( HUDWEAPON_MOVEX > 4 )
			{
				HUDWEAPON_MOVEX = 4;
				HUDWEAPON_CHOP++;
			}
		}
	}
	else if ( HUDWEAPON_CHOP == 9 )     // return from third swing
	{
		if ( playerRace == SPIDER )
		{
			HUDWEAPON_MOVEX -= .25;
		}
		else
		{
			HUDWEAPON_MOVEX -= .5;
		}
		if ( HUDWEAPON_MOVEX < 0 )
		{
			HUDWEAPON_MOVEX = 0;
		}
		HUDWEAPON_MOVEY += .25;
		if ( HUDWEAPON_MOVEY > 0 )
		{
			HUDWEAPON_MOVEY = 0;
		}
		HUDWEAPON_MOVEZ += .35;
		if ( HUDWEAPON_MOVEZ > 0 )
		{
			HUDWEAPON_MOVEZ = 0;
		}
		if ( HUDWEAPON_MOVEX == 0 )
		{
			if ( swingweapon )
			{
				// restart the combo...
				if ( stats[clientnum]->weapon == NULL || hideWeapon )
				{
					HUDWEAPON_CHOP = 7;
				}
				else
				{
					if ( itemCategory(stats[clientnum]->weapon) != MAGICSTAFF && stats[clientnum]->weapon->type != CRYSTAL_SPEAR && stats[clientnum]->weapon->type != IRON_SPEAR && stats[clientnum]->weapon->type != ARTIFACT_SPEAR )
					{
						HUDWEAPON_CHOP = 1;
					}
					else
					{
						HUDWEAPON_CHOP = 7;
					}
				}
			}
			HUDWEAPON_YAW -= .25;
			if ( HUDWEAPON_YAW < -.1 )
			{
				HUDWEAPON_YAW = -.1;
			}
			if ( playerRace == SPIDER )
			{
				HUDWEAPON_PITCH += .12;
			}
			else
			{
				HUDWEAPON_PITCH += .05;
			}
			if ( HUDWEAPON_PITCH > 0)
			{
				HUDWEAPON_PITCH = 0;
			}
			HUDWEAPON_ROLL += .25;
			if ( HUDWEAPON_ROLL > 0 )
			{
				HUDWEAPON_ROLL = 0;
				if ( HUDWEAPON_YAW == -.1 && HUDWEAPON_PITCH == 0 && HUDWEAPON_MOVEZ == 0 && HUDWEAPON_MOVEY == 0 && HUDWEAPON_MOVEX == 0 )
				{
					HUDWEAPON_CHOP = 0;
				}
			}
		}
	}
	else if ( HUDWEAPON_CHOP == 10 ) // special punch
	{
		HUDWEAPON_MOVEX = std::max(static_cast<real_t>(0.0), HUDWEAPON_MOVEX - .15); // forward/back
		HUDWEAPON_MOVEY = std::min(static_cast<real_t>(1.0), HUDWEAPON_MOVEY + .25); // left/right
		HUDWEAPON_MOVEZ = std::max(static_cast<real_t>(-2.0), HUDWEAPON_MOVEZ - .05); // up/down

		HUDWEAPON_YAW -= .15;
		if ( HUDWEAPON_YAW < 2 * PI / 5 )
		{
			result = 2 * PI / 5;
			HUDWEAPON_YAW = result;
		}
		real_t pitchLimit = .2;
		HUDWEAPON_PITCH -= .05;
		if ( HUDWEAPON_PITCH < pitchLimit )
		{
			HUDWEAPON_PITCH = pitchLimit;
		}
		HUDWEAPON_ROLL -= .15;
		if ( HUDWEAPON_ROLL < -2 * PI / 5 )
		{
			HUDWEAPON_ROLL = -2 * PI / 5;
			if ( HUDWEAPON_CHARGE < 40 )
			{
				HUDWEAPON_CHARGE += 1;
			}
			else
			{
				HUDWEAPON_CHOP = 11;
				HUDWEAPON_CHARGE = 0;
				HUDWEAPON_OVERCHARGE = 0;
				if ( players[clientnum]->entity->skill[3] == 0 )   // debug cam OFF
				{
					camera_shakex += .06;
					camera_shakey += 6;
				}
				players[clientnum]->entity->attack(PLAYER_POSE_GOLEM_SMASH, MAXCHARGE, nullptr);
			}
		}
	}
	else if ( HUDWEAPON_CHOP == 11 )     // third swing
	{
		HUDWEAPON_MOVEX += 1;
		if ( HUDWEAPON_MOVEX > 4 )
		{
			HUDWEAPON_MOVEX = 4;
			HUDWEAPON_CHOP = 12;
		}
	}
	else if ( HUDWEAPON_CHOP == 12 )     // return from third swing
	{
		HUDWEAPON_MOVEX -= .2;
		if ( HUDWEAPON_MOVEX < 0 )
		{
			HUDWEAPON_MOVEX = 0;
		}
		HUDWEAPON_MOVEY += .15;
		if ( HUDWEAPON_MOVEY > 0 )
		{
			HUDWEAPON_MOVEY = 0;
		}
		HUDWEAPON_MOVEZ += .25;
		if ( HUDWEAPON_MOVEZ > 0 )
		{
			HUDWEAPON_MOVEZ = 0;
		}
		if ( HUDWEAPON_MOVEX == 0 )
		{
			HUDWEAPON_YAW -= .25;
			if ( HUDWEAPON_YAW < -.1 )
			{
				HUDWEAPON_YAW = -.1;
			}
			HUDWEAPON_PITCH += .05;
			if ( HUDWEAPON_PITCH > 0 )
			{
				HUDWEAPON_PITCH = 0;
			}
			HUDWEAPON_ROLL += .25;
			if ( HUDWEAPON_ROLL > 0 )
			{
				HUDWEAPON_ROLL = 0;
				if ( HUDWEAPON_YAW == -.1 && HUDWEAPON_PITCH == 0 && HUDWEAPON_MOVEZ == 0 && HUDWEAPON_MOVEY == 0 && HUDWEAPON_MOVEX == 0 )
				{
					HUDWEAPON_CHOP = 0;
				}
			}
		}
	}
	else if ( HUDWEAPON_CHOP == 13 ) // tool placing
	{
		int targetZ = -4;
		real_t targetRoll = -PI / 2;
		real_t rateY = .1;
		real_t rateRoll = .25;
		int targetY = 1;
		if ( !swingweapon )
		{
			//targetY = 0;
		}
		real_t targetPitch = 0.f;

		HUDWEAPON_YAW = 0;
		HUDWEAPON_PITCH -= .25;
		if ( HUDWEAPON_PITCH < targetPitch )
		{
			HUDWEAPON_PITCH = targetPitch;
		}
		HUDWEAPON_MOVEX -= .35;
		if ( HUDWEAPON_MOVEX < 0 )
		{
			HUDWEAPON_MOVEX = 0;
		}
		HUDWEAPON_MOVEZ -= .75;
		if ( HUDWEAPON_MOVEZ < targetZ )
		{
			HUDWEAPON_MOVEZ = targetZ;
		}
		HUDWEAPON_MOVEY += rateY;
		if ( HUDWEAPON_MOVEY > targetY )
		{
			HUDWEAPON_MOVEY = targetY;
		}
		HUDWEAPON_ROLL -= rateRoll;

		if ( HUDWEAPON_ROLL < targetRoll )
		{
			HUDWEAPON_ROLL = targetRoll;
			if ( HUDWEAPON_PITCH == targetPitch && HUDWEAPON_MOVEX == 0 && HUDWEAPON_MOVEY == targetY && HUDWEAPON_MOVEZ == targetZ )
			{
				if ( !swingweapon )
				{
					HUDWEAPON_CHOP = 14;
					if ( players[clientnum]->entity->skill[3] == 0 )   // debug cam OFF
					{
						camera_shakex += .07;
					}
					Entity* player = players[clientnum]->entity;
					if ( stats[clientnum]->weapon && player )
					{
						//lineTrace(player, player->x, player->y, player->yaw, STRIKERANGE, 0, false);
						players[clientnum]->entity->attack(2, HUDWEAPON_CHARGE, nullptr);
						throwGimpTimer = TICKS_PER_SECOND / 2; // limits how often you can throw objects
						Item* item = stats[clientnum]->weapon;
						if ( multiplayer == CLIENT )
						{
							item->count--;
							if ( item->count <= 0 )
							{
								if ( item->node )
								{
									list_RemoveNode(item->node);
								}
								else
								{
									free(item);
								}
								stats[clientnum]->weapon = NULL;
							}
						}
						if ( !stats[clientnum]->weapon )
						{
							HUDWEAPON_ROLL = 0;
							HUDWEAPON_MOVEZ = 3;
						}
					}
					HUDWEAPON_CHARGE = 0;
					HUDWEAPON_OVERCHARGE = 0;
				}
				else
				{
					HUDWEAPON_CHARGE = std::min<real_t>(HUDWEAPON_CHARGE + 1, MAXCHARGE);
				}
			}
		}
	}
	else if ( HUDWEAPON_CHOP == 14 )     // second swing
	{
		HUDWEAPON_MOVEX = sin(HUDWEAPON_YAW) * 2;
		HUDWEAPON_MOVEY = cos(HUDWEAPON_YAW) * 1;
		HUDWEAPON_YAW += .3;
		if ( HUDWEAPON_YAW > (2 * PI) / 4 )
		{
			HUDWEAPON_YAW = (2 * PI) / 4;
			HUDWEAPON_CHOP++;
		}
	}
	else if ( HUDWEAPON_CHOP == 15 )     // return from second swing
	{
		HUDWEAPON_MOVEX -= .25;
		if ( HUDWEAPON_MOVEX < 0 )
		{
			HUDWEAPON_MOVEX = 0;
		}
		HUDWEAPON_MOVEY -= .25;
		if ( HUDWEAPON_MOVEY < 0 )
		{
			HUDWEAPON_MOVEY = 0;
		}
		HUDWEAPON_ROLL += .05;

		HUDWEAPON_YAW -= .05;
		if ( HUDWEAPON_YAW < -.1 )
		{
			HUDWEAPON_YAW = -.1;
		}
		HUDWEAPON_MOVEZ += .35;
		if ( HUDWEAPON_MOVEZ > 0 )
		{
			HUDWEAPON_MOVEZ = 0;
		}
		if ( HUDWEAPON_ROLL > 0 )
		{
			HUDWEAPON_ROLL = 0;
			if ( HUDWEAPON_YAW == -.1 && HUDWEAPON_MOVEZ == 0 && HUDWEAPON_MOVEY == 0 && HUDWEAPON_MOVEX == 0 )
			{
				HUDWEAPON_CHOP = 0;
			}
		}
	}
	else if ( HUDWEAPON_CHOP == 16 )     // crossbow reload
	{
		if ( HUDWEAPON_MOVEX > 0 )
		{
			HUDWEAPON_MOVEX = std::max<real_t>(HUDWEAPON_MOVEX - 1, 0.0);
		}
		else if ( HUDWEAPON_MOVEX < 0 )
		{
			HUDWEAPON_MOVEX = std::min<real_t>(HUDWEAPON_MOVEX + .15, 0.0);
			if ( HUDWEAPON_MOVEX > -1 )
			{
				HUDWEAPON_RANGED_QUIVER_RELOAD = 2;
				HUDWEAPON_MOVEZ += .15;
				if ( HUDWEAPON_MOVEZ > 1 )
				{
					HUDWEAPON_RANGED_QUIVER_RELOAD = 3;
					HUDWEAPON_MOVEZ = 1;
				}
			}
		}
		if ( fabs(HUDWEAPON_MOVEX) < 0.01 )
		{
			HUDWEAPON_CHOP = 17;
		}
		if ( HUDWEAPON_ROLL > 0 )
		{
			HUDWEAPON_ROLL = std::max<real_t>(HUDWEAPON_ROLL - .1, 0.0);
		}
		else if ( HUDWEAPON_ROLL < 0 )
		{
			HUDWEAPON_ROLL = std::min<real_t>(HUDWEAPON_ROLL + .1, 0.0);
		}
	}
	else if ( HUDWEAPON_CHOP == 17 )     // crossbow reload
	{
		if ( HUDWEAPON_MOVEZ > 1 )
		{
			HUDWEAPON_MOVEZ = std::max<real_t>(HUDWEAPON_MOVEZ - 1, 1);
		}
		HUDWEAPON_MOVEZ -= .1;
		if ( HUDWEAPON_MOVEZ < 0 )
		{
			HUDWEAPON_MOVEZ = 0;
			HUDWEAPON_CHOP = 0;
		}
		if ( HUDWEAPON_ROLL > 0 )
		{
			HUDWEAPON_ROLL = std::max<real_t>(HUDWEAPON_ROLL - .1, 0.0);
		}
		else if ( HUDWEAPON_ROLL < 0 )
		{
			HUDWEAPON_ROLL = std::min<real_t>(HUDWEAPON_ROLL + .1, 0.0);
		}
	}

	if ( HUDWEAPON_CHARGE == MAXCHARGE || castStrikeAnimation 
		|| players[clientnum]->entity->skill[9] == MONSTER_POSE_SPECIAL_WINDUP2
		|| shakeRangedWeapon )
	{
		if ( ticks % 5 == 0 && players[clientnum]->entity->skill[9] == MONSTER_POSE_SPECIAL_WINDUP2 )
		{
			camera_shakey += 6;
		}
		if ( ticks % 2 == 0 )
		{
			// charge vibration
			HUDWEAPON_MOVEX -= HUDWEAPON_OLDVIBRATEX;
			HUDWEAPON_MOVEY -= HUDWEAPON_OLDVIBRATEY;
			HUDWEAPON_MOVEZ -= HUDWEAPON_OLDVIBRATEZ;
			HUDWEAPON_OLDVIBRATEX = (rand() % 30 - 10) / 80.f;
			HUDWEAPON_OLDVIBRATEY = (rand() % 30 - 10) / 80.f;
			HUDWEAPON_OLDVIBRATEZ = (rand() % 30 - 10) / 80.f;
			HUDWEAPON_MOVEX += HUDWEAPON_OLDVIBRATEX;
			HUDWEAPON_MOVEY += HUDWEAPON_OLDVIBRATEY;
			HUDWEAPON_MOVEZ += HUDWEAPON_OLDVIBRATEZ;
		}
		if ( (castStrikeAnimation || players[clientnum]->entity->skill[9] == MONSTER_POSE_SPECIAL_WINDUP2)
			|| shakeRangedWeapon )
		{
			// don't overcharge here.
		}
		else
		{
			HUDWEAPON_OVERCHARGE++; 
		}
	}
	if ( castStrikeAnimation ) // magic sprite particles around the fist
	{
		if ( ticks % 5 == 0 )
		{
			Entity* entity = spawnGib(my);
			entity->flags[INVISIBLE] = false;
			entity->flags[SPRITE] = true;
			entity->flags[NOUPDATE] = true;
			entity->flags[UPDATENEEDED] = false;
			entity->flags[OVERDRAW] = true;
			entity->flags[BRIGHT] = true;
			entity->scalex = 0.25f; //MAKE 'EM SMALL PLEASE!
			entity->scaley = 0.25f;
			entity->scalez = 0.25f;
			entity->z -= 3.5;
			entity->sprite = 16; //TODO: Originally. 22. 16 -- spark sprite instead?
			entity->yaw = ((rand() % 6) * 60) * PI / 180.0;
			entity->pitch = (rand() % 360) * PI / 180.0;
			entity->roll = (rand() % 360) * PI / 180.0;
			entity->vel_x = cos(entity->yaw) * .1;
			entity->vel_y = sin(entity->yaw) * .1;
			entity->vel_z = -.15;
			entity->fskill[3] = 0.01;
		}
	}

	// move the weapon
	if (players[clientnum] == nullptr || players[clientnum]->entity == nullptr)
	{
		return;
	}
	double defaultpitch = PI / 8.f;
	if (stats[clientnum]->weapon == nullptr || hideWeapon)
	{
		if ( playerRace == RAT )
		{
			my->x = 6 + HUDWEAPON_MOVEX / 3;
			my->y = 3;// +HUDWEAPON_MOVEY;
			my->z = (camera.z * .1 - players[clientnum]->entity->z) + 7 + HUDWEAPON_MOVEZ / 10;
			my->yaw = 0.f - camera_shakex2;
			my->pitch = defaultpitch + HUDWEAPON_PITCH - camera_shakey2 / 200.f;
			my->roll = HUDWEAPON_ROLL;

			if ( isLevitating(stats[clientnum]) )
			{
				my->z -= 2 * .4;
			}
		}
		else if ( playerRace == SPIDER )
		{
			my->x = 6 + HUDWEAPON_MOVEX;
			my->y = 3;// +HUDWEAPON_MOVEY;
			my->z = (camera.z * .5 - players[clientnum]->entity->z) + 7 + HUDWEAPON_MOVEZ;
			my->yaw = 0.f - camera_shakex2;
			my->pitch = defaultpitch + HUDWEAPON_PITCH - camera_shakey2 / 200.f;
			my->roll = HUDWEAPON_ROLL;
		}
		else
		{
			my->x = 6 + HUDWEAPON_MOVEX;
			my->y = 3 + HUDWEAPON_MOVEY;
			my->z = (camera.z * .5 - players[clientnum]->entity->z) + 7 + HUDWEAPON_MOVEZ;
			my->yaw = HUDWEAPON_YAW - camera_shakex2;
			my->pitch = defaultpitch + HUDWEAPON_PITCH - camera_shakey2 / 200.f;
			my->roll = HUDWEAPON_ROLL;
		}

		// dirty hack because we altered the camera height in actPlayer(). adjusts HUD to match new height.
		if ( playerRace == CREATURE_IMP && players[clientnum]->entity->z == -4.5 )
		{
			my->z -= .5;
		}
		else if ( playerRace == TROLL && players[clientnum]->entity->z <= -1.5 )
		{
			my->z -= -2 * .5;
		}
	}
	else
	{
		Item* item = stats[clientnum]->weapon;
		if (item)
		{
			if (item->type == TOOL_SKELETONKEY || item->type == TOOL_LOCKPICK
				|| (item->type >= ARTIFACT_ORB_BLUE && item->type <= ARTIFACT_ORB_GREEN))
			{
				defaultpitch = -PI / 8.f;
			}
			else if ( item->type == TOOL_WHIP )
			{
				defaultpitch = -PI / 8.f;
			}
			if (item->type == CROSSBOW)
			{
				my->x = 6 + HUDWEAPON_MOVEX;
				my->y = 1.5 + HUDWEAPON_MOVEY;
				my->z = (camera.z * .5 - players[clientnum]->entity->z) + 8 + HUDWEAPON_MOVEZ;
				my->yaw = -.05 - camera_shakex2;
				my->pitch = HUDWEAPON_PITCH - camera_shakey2 / 200.f;
				my->roll = HUDWEAPON_ROLL;
			}
			else if ( item->type == SLING
				|| item->type == SHORTBOW
				|| item->type == ARTIFACT_BOW
				|| item->type == LONGBOW
				|| item->type == COMPOUND_BOW )
			{
				my->x = 6 + HUDWEAPON_MOVEX;
				my->y = 3 + HUDWEAPON_MOVEY;
				my->z = (camera.z * .5 - players[clientnum]->entity->z) + 7 + HUDWEAPON_MOVEZ;
				my->yaw = HUDWEAPON_YAW - camera_shakex2;
				my->pitch = HUDWEAPON_PITCH - camera_shakey2 / 200.f;
				my->roll = HUDWEAPON_ROLL;
			}
			else if ( item->type == TOOL_WHIP )
			{
				my->x = 6 + HUDWEAPON_MOVEX + 5;
				my->y = 3 + HUDWEAPON_MOVEY - 0.5 + 1;
				//my->flags[OVERDRAW] = false;
				my->z = (camera.z * .5 - players[clientnum]->entity->z) + 7 + HUDWEAPON_MOVEZ;
				my->yaw = HUDWEAPON_YAW - camera_shakex2;
				my->pitch = defaultpitch + HUDWEAPON_PITCH - camera_shakey2 / 200.f;
				if ( my->sprite == items[TOOL_WHIP].fpindex + 1 )
				{
					my->pitch -= PI / 4;
					my->focalx = 10 + (HUDWEAPON_CHOP == 5 ? 4 : 0);
					my->focaly = 0;
					my->focalz = -4;
					my->x += (HUDWEAPON_CHOP == 5 ? -3 : 0);
					my->y += (HUDWEAPON_CHOP == 5 ? 6 : 0);
				}
				else
				{
					my->focalx = 0;
					my->focaly = 0;
					my->focalz = -4;
				}
				my->roll = HUDWEAPON_ROLL;
			}
			else
			{
				my->x = 6 + HUDWEAPON_MOVEX + 3 * (itemCategory(item) == POTION);
				my->y = 3 + HUDWEAPON_MOVEY - 3 * (itemCategory(item) == POTION);
				my->z = (camera.z * .5 - players[clientnum]->entity->z) + 7 + HUDWEAPON_MOVEZ - 3 * (itemCategory(item) == POTION);
				my->yaw = HUDWEAPON_YAW - camera_shakex2;
				my->pitch = defaultpitch + HUDWEAPON_PITCH - camera_shakey2 / 200.f;
				my->roll = HUDWEAPON_ROLL + (PI / 2) * (itemCategory(item) == POTION);
				my->focalx = 0;
			}

			if ( item->type >= TOOL_BOMB && item->type <= TOOL_TELEPORT_BOMB )
			{
				my->z += .5;
			}
		}

		// dirty hack because we altered the camera height in actPlayer(). adjusts HUD to match new height.
		if ( playerRace == CREATURE_IMP && players[clientnum]->entity->z == -4.5 )
		{
			my->z -= .5;
		}
		else if ( playerRace == TROLL && players[clientnum]->entity->z <= -1.5 )
		{
			my->z -= -2 * .5;
		}
	}
	if ( !my->flags[OVERDRAW] )
	{
		my->x += 32;
		my->y += 32;
		my->z -= 3.5;
	}
}

#define HUDSHIELD_DEFEND my->skill[0]
#define HUDSHIELD_SNEAKING my->skill[1]
#define HUDSHIELD_MOVEX my->fskill[0]
#define HUDSHIELD_MOVEY my->fskill[1]
#define HUDSHIELD_MOVEZ my->fskill[2]
#define HUDSHIELD_YAW my->fskill[3]
#define HUDSHIELD_PITCH my->fskill[4]
#define HUDSHIELD_ROLL my->fskill[5]

void actHudShield(Entity* my)
{
	my->flags[UNCLICKABLE] = true;

	// isn't active during intro/menu sequence
	if (intro == true)
	{
		my->flags[INVISIBLE] = true;
		return;
	}

	if (multiplayer == CLIENT)
	{
		if (stats[clientnum]->HP <= 0)
		{
			my->flags[INVISIBLE] = true;
			return;
		}
	}

	// this entity only exists so long as the player exists
	if (players[clientnum] == nullptr || players[clientnum]->entity == nullptr || !hudweapon)
	{
		list_RemoveNode(my->mynode);
		return;
	}

	// check levitating value
	bool levitating = isLevitating(stats[clientnum]);

	// water walking boots
	bool waterwalkingboots = false;
	if (stats[clientnum]->shoes != nullptr)
		if (stats[clientnum]->shoes->type == IRON_BOOTS_WATERWALKING)
		{
			waterwalkingboots = true;
		}

	// select model
	bool wearingring = false;
	if ( stats[clientnum]->ring != nullptr )
	{
		if ( stats[clientnum]->ring->type == RING_INVISIBILITY )
		{
			wearingring = true;
		}
	}
	if ( stats[clientnum]->cloak != nullptr )
	{
		if ( stats[clientnum]->cloak->type == CLOAK_INVISIBILITY )
		{
			wearingring = true;
		}
	}

	Monster playerRace = players[clientnum]->entity->getMonsterFromPlayerRace(stats[clientnum]->playerRace);
	if ( players[clientnum]->entity->effectShapeshift != NOTHING )
	{
		playerRace = static_cast<Monster>(players[clientnum]->entity->effectShapeshift);
	}
	else if ( players[clientnum]->entity->effectPolymorph != NOTHING )
	{
		if ( players[clientnum]->entity->effectPolymorph > NUMMONSTERS )
		{
			playerRace = HUMAN;
		}
		else
		{
			playerRace = static_cast<Monster>(players[clientnum]->entity->effectPolymorph);
		}
	}

	bool hideShield = false;
	if ( playerRace == RAT
		|| playerRace == CREATURE_IMP
		|| playerRace == TROLL )
	{
		hideShield = true;
	}

	bool spellbook = false;
	bool quiver = false;
	if ( stats[clientnum]->shield && itemCategory(stats[clientnum]->shield) == SPELLBOOK )
	{
		spellbook = true;
		if ( playerRace == CREATURE_IMP )
		{
			hideShield = false;
		}
	}
	else if ( stats[clientnum]->shield && itemTypeIsQuiver(stats[clientnum]->shield->type) )
	{
		quiver = true;
	}

	if ( players[clientnum]->entity->skill[3] == 1 || players[clientnum]->entity->isInvisible() )   // debug cam or player invisible
	{
		my->flags[INVISIBLE] = true;
	}
	else
	{
		if (stats[clientnum]->shield == nullptr && playerRace != SPIDER )
		{
			my->flags[INVISIBLE] = true;
		}
		else
		{
			if (stats[clientnum]->shield)
			{
				if (itemModelFirstperson(stats[clientnum]->shield) != itemModel(stats[clientnum]->shield))
				{
					if ( spellbook )
					{
						my->scalex = 0.35;
						my->scaley = 0.35;
						my->scalez = 0.35;
					}
					else if ( quiver )
					{
						my->scalex = 0.5f;
						my->scaley = 0.5f;
						my->scalez = 0.5f;
					}
					else
					{
						my->scalex = 0.5f;
						my->scaley = 0.5f;
						my->scalez = 0.5f;
					}
				}
				else
				{
					my->scalex = 1.f;
					my->scaley = 1.f;
					my->scalez = 1.f;
				}
			}
			my->sprite = itemModelFirstperson(stats[clientnum]->shield);
			my->flags[INVISIBLE] = false;
		}
	}

	// swimming
	bool swimming = false;
	if (players[clientnum] && players[clientnum]->entity)
	{
		if (!levitating && !waterwalkingboots && !skillCapstoneUnlocked(clientnum, PRO_SWIMMING) )
		{
			int x = std::min<int>(std::max<int>(0, floor(players[clientnum]->entity->x / 16)), map.width - 1);
			int y = std::min<int>(std::max<int>(0, floor(players[clientnum]->entity->y / 16)), map.height - 1);
			if ( swimmingtiles[map.tiles[y * MAPLAYERS + x * MAPLAYERS * map.height]] || lavatiles[map.tiles[y * MAPLAYERS + x * MAPLAYERS * map.height]] )
			{
				my->flags[INVISIBLE] = true;
				Entity* parent = uidToEntity(my->parent);
				if (parent)
				{
					parent->flags[INVISIBLE] = true;
				}
				swimming = true;
			}
		}
	}

	if ( hideShield )
	{
		my->flags[INVISIBLE] = true;
	}
	else if ( cast_animation.active )
	{
		my->flags[INVISIBLE] = true;
	}
	else if ( cast_animation.active_spellbook && !spellbook )
	{
		my->flags[INVISIBLE] = true;
	}

	bool defending = false;
	bool sneaking = false;
	if (!command && !swimming)
	{
		if ( players[clientnum] && players[clientnum]->entity 
			&& (*inputPressed(impulses[IN_DEFEND]) || (shootmode && *inputPressed(joyimpulses[INJOY_GAME_DEFEND]))) 
			&& players[clientnum]->entity->isMobile() 
			&& !gamePaused 
			&& !cast_animation.active
			&& !cast_animation.active_spellbook
			&& (!spellbook || (spellbook && hideShield)) )
		{
			if ( stats[clientnum]->shield && (hudweapon->skill[0] % 3 == 0) )
			{
				defending = true;
			}
			sneaking = true;
		}
	}

	if ( stats[clientnum]->shield && itemTypeIsQuiver(stats[clientnum]->shield->type) )
	{
		// can't defend with quivers.
		defending = false;
	}
	if ( playerRace == RAT
		|| playerRace == CREATURE_IMP
		|| playerRace == TROLL
		|| playerRace == SPIDER )
	{
		defending = false;
	}

	if (defending)
	{
		stats[clientnum]->defending = true;
	}
	else
	{
		stats[clientnum]->defending = false;
	}
	if ( sneaking )
	{
		stats[clientnum]->sneaking = true;
	}
	else
	{
		stats[clientnum]->sneaking = false;
	}

	if (multiplayer == CLIENT)
	{
		if (HUDSHIELD_DEFEND != defending || ticks % 120 == 0)
		{
			strcpy((char*)net_packet->data, "SHLD");
			net_packet->data[4] = clientnum;
			net_packet->data[5] = defending;
			net_packet->address.host = net_server.host;
			net_packet->address.port = net_server.port;
			net_packet->len = 6;
			sendPacketSafe(net_sock, -1, net_packet, 0);
		}
		if ( HUDSHIELD_SNEAKING != sneaking || ticks % 120 == 0 )
		{
			strcpy((char*)net_packet->data, "SNEK");
			net_packet->data[4] = clientnum;
			net_packet->data[5] = sneaking;
			net_packet->address.host = net_server.host;
			net_packet->address.port = net_server.port;
			net_packet->len = 6;
			sendPacketSafe(net_sock, -1, net_packet, 0);
		}
	}
	HUDSHIELD_DEFEND = defending;
	HUDSHIELD_SNEAKING = sneaking;

	bool crossbow = (stats[clientnum]->weapon && stats[clientnum]->weapon->type == CROSSBOW);
	bool doCrossbowReloadAnimation = false;

	// shield switching animation
	if ( shieldSwitch )
	{
		if ( !spellbook )
		{
			shieldSwitch = false;
		}
		if ( !(defending || (spellbook && cast_animation.active_spellbook)) )
		{
			HUDSHIELD_MOVEY = -6;
			HUDSHIELD_MOVEZ = 2;
			HUDSHIELD_MOVEX = -2;
		}
		if ( hudweapon && crossbow )
		{
			if ( rangedWeaponUseQuiverOnAttack(stats[clientnum]) )
			{
				doCrossbowReloadAnimation = true;
			}
		}
	}

	bool crossbowReloadAnimation = true;
	if ( hudweapon && crossbow 
		&& hudweapon->skill[8] == 4 && rangedWeaponUseQuiverOnAttack(stats[clientnum]) )
	{
		if ( fabs(hudweapon->fskill[5]) < (3 * PI / 8) )
		{
			doCrossbowReloadAnimation = true;
		}
		else
		{
			crossbowReloadAnimation = false;
		}
	}

	// main animation
	if ( defending || cast_animation.active_spellbook )
	{
		if ( !spellbook )
		{
			if ( HUDSHIELD_MOVEY < 3 )
			{
				HUDSHIELD_MOVEY += .5;
				if ( HUDSHIELD_MOVEY > 3 )
				{
					HUDSHIELD_MOVEY = 3;
				}
			}
			if ( HUDSHIELD_MOVEZ > -1 )
			{
				HUDSHIELD_MOVEZ -= .2;
				if ( HUDSHIELD_MOVEZ < -1 )
				{
					HUDSHIELD_MOVEZ = -1;
				}
			}
			if ( HUDSHIELD_YAW < PI / 3 )
			{
				HUDSHIELD_YAW += .15;
				if ( HUDSHIELD_YAW > PI / 3 )
				{
					HUDSHIELD_YAW = PI / 3;
				}
			}
			if ( stats[clientnum]->shield )
			{
				if ( stats[clientnum]->shield->type == TOOL_TORCH || stats[clientnum]->shield->type == TOOL_CRYSTALSHARD )
				{
					if ( HUDSHIELD_MOVEX < 1.5 )
					{
						HUDSHIELD_MOVEX += .5;
						if ( HUDSHIELD_MOVEX > 1.5 )
						{
							HUDSHIELD_MOVEX = 1.5;
						}
					}
					if ( HUDSHIELD_ROLL < PI / 5 )
					{
						HUDSHIELD_ROLL += .15;
						if ( HUDSHIELD_ROLL > PI / 5 )
						{
							HUDSHIELD_ROLL = PI / 5;
						}
					}
				}
			}
		}
		else
		{
			if ( cast_animation.active_spellbook )
			{
				HUDSHIELD_ROLL = std::min<real_t>(HUDSHIELD_ROLL + .15, PI / 2);
				if ( HUDSHIELD_MOVEZ > -1 )
				{
					HUDSHIELD_MOVEZ -= .2;
					if ( HUDSHIELD_MOVEZ < -1 )
					{
						HUDSHIELD_MOVEZ = -1;
					}
				}
			}
		}
	}
	else if ( !hideShield && quiver && hudweapon 
		&& hudweapon->skill[7] != 0 
		&& (!crossbow || (crossbow && crossbowReloadAnimation && hudweapon->skill[8] != 2)) ) // skill[7] == 1 is hudweapon bow drawing
	{
		if ( hudweapon->skill[7] == 2 && (!crossbow || (crossbow && hudweapon->skill[8] == 1)) )
		{
			my->flags[INVISIBLE] = true;
			HUDSHIELD_MOVEY = 0;
			HUDSHIELD_PITCH = 0;
			HUDSHIELD_YAW = 0;
			HUDSHIELD_MOVEZ = 0;
			HUDSHIELD_MOVEX = 0;

			if ( crossbow && hudweapon->skill[8] == 1 )
			{
				HUDSHIELD_MOVEZ = 2;
			}
		}

		real_t targetY = 5.05;// +limbs[HUMAN][11][0];
		real_t targetPitch = PI / 2;// +limbs[HUMAN][11][1];
		real_t targetYaw = PI / 3 - 0.1;// +limbs[HUMAN][11][2];
		real_t targetZ = -3.5;// +limbs[HUMAN][12][0];
		real_t targetX = -1.75;// +limbs[HUMAN][12][1];

		if ( crossbow )
		{
			targetY = 4.8 + 0.01;
			targetPitch = PI / 2;
			targetYaw = PI / 3 - 0.05;
			targetZ = -2.75;
			targetX = 2.75;

			if ( stats[clientnum]->shield->type == QUIVER_LIGHTWEIGHT )
			{
				targetZ -= 0.25; // offset a bit higher.
			}

			if ( doCrossbowReloadAnimation )
			{
				hudweapon->skill[8] = 2;
				hudweapon->skill[0] = 16;
				hudweapon->fskill[0] = -1;
				throwGimpTimer = std::max(throwGimpTimer, 20);

				if ( fabs(hudweapon->fskill[5]) < 0.01 )
				{
					throwGimpTimer = std::max(throwGimpTimer, 20);
					my->flags[INVISIBLE] = true;
					HUDSHIELD_MOVEY = 0;
					HUDSHIELD_PITCH = 0;
					HUDSHIELD_YAW = 0;
					HUDSHIELD_MOVEZ = 2;
					HUDSHIELD_MOVEX = 0;
				}
			}
		}

		bool doMovement = true;
		if ( doCrossbowReloadAnimation )
		{
			doMovement = false;
		}
		if ( doMovement )
		{
			if ( HUDSHIELD_MOVEY < targetY )
			{
				HUDSHIELD_MOVEY += 0.5;
				if ( HUDSHIELD_MOVEY > targetY )
				{
					HUDSHIELD_MOVEY = targetY;
				}
			}
			if ( targetX < 0 )
			{
				if ( HUDSHIELD_MOVEX > targetX )
				{
					HUDSHIELD_MOVEX -= 0.4;
					if ( HUDSHIELD_MOVEX < targetX )
					{
						HUDSHIELD_MOVEX = targetX;
					}
				}
			}
			else if ( targetX >= 0 )
			{
				if ( HUDSHIELD_MOVEX < targetX )
				{
					HUDSHIELD_MOVEX += 0.4;
					if ( HUDSHIELD_MOVEX > targetX )
					{
						HUDSHIELD_MOVEX = targetX;
					}
				}
			}

			if ( HUDSHIELD_PITCH < targetPitch )
			{
				HUDSHIELD_PITCH += .2;
				if ( HUDSHIELD_PITCH > targetPitch )
				{
					HUDSHIELD_PITCH = targetPitch;
				}
			}
			if ( HUDSHIELD_YAW < targetYaw )
			{
				HUDSHIELD_YAW += .3;
				if ( HUDSHIELD_YAW > targetYaw )
				{
					HUDSHIELD_YAW = targetYaw;
				}
			}
			if ( HUDSHIELD_MOVEZ > targetZ )
			{
				HUDSHIELD_MOVEZ -= 0.4;
				if ( HUDSHIELD_MOVEZ < targetZ )
				{
					HUDSHIELD_MOVEZ = targetZ;
				}
			}
		}
	}
	else
	{
		if ( HUDSHIELD_MOVEX > 0 )
		{
			HUDSHIELD_MOVEX = std::max<real_t>(HUDSHIELD_MOVEX - .5, 0.0);
		}
		else if ( HUDSHIELD_MOVEX < 0 )
		{
			HUDSHIELD_MOVEX = std::min<real_t>(HUDSHIELD_MOVEX + .5, 0.0);
		}
		if ( HUDSHIELD_MOVEY > 0 )
		{
			HUDSHIELD_MOVEY = std::max<real_t>(HUDSHIELD_MOVEY - .5, 0.0);
		}
		else if ( HUDSHIELD_MOVEY < 0 )
		{
			HUDSHIELD_MOVEY = std::min<real_t>(HUDSHIELD_MOVEY + .5, 0.0);
		}
		if ( HUDSHIELD_MOVEZ > 0 )
		{
			HUDSHIELD_MOVEZ = std::max<real_t>(HUDSHIELD_MOVEZ - .2, 0.0);
		}
		else if ( HUDSHIELD_MOVEZ < 0 )
		{
			HUDSHIELD_MOVEZ = std::min<real_t>(HUDSHIELD_MOVEZ + .2, 0.0);
		}
		if ( HUDSHIELD_YAW > 0 )
		{
			HUDSHIELD_YAW = std::max<real_t>(HUDSHIELD_YAW - .15, 0.0);
		}
		else if ( HUDSHIELD_YAW < 0 )
		{
			HUDSHIELD_YAW = std::min<real_t>(HUDSHIELD_YAW + .15, 0.0);
		}
		if ( HUDSHIELD_PITCH > 0 )
		{
			HUDSHIELD_PITCH = std::max<real_t>(HUDSHIELD_PITCH - .15, 0.0);
		}
		else if ( HUDSHIELD_PITCH < 0 )
		{
			HUDSHIELD_PITCH = std::min<real_t>(HUDSHIELD_PITCH + .15, 0.0);
		}
		if ( HUDSHIELD_ROLL > 0 )
		{
			HUDSHIELD_ROLL = std::max<real_t>(HUDSHIELD_ROLL - .15, 0);
		}
		else if ( HUDSHIELD_ROLL < 0 )
		{
			HUDSHIELD_ROLL = std::min<real_t>(HUDSHIELD_ROLL + .15, 0);
		}
	}

	// set entity position
	my->x = 7 + HUDSHIELD_MOVEX;
	my->y = -3.5 + HUDSHIELD_MOVEY;
	my->z = 6 + HUDSHIELD_MOVEZ + (camera.z * .5 - players[clientnum]->entity->z);
	if ( !my->flags[OVERDRAW] )
	{
		my->x += 32;
		my->y += 32;
		my->z -= 3.5;
	}
	my->yaw = HUDSHIELD_YAW - camera_shakex2 - PI / 3;
	my->pitch = HUDSHIELD_PITCH - camera_shakey2 / 200.f;
	my->roll = HUDSHIELD_ROLL;
	if ( spellbook )
	{
		my->x += 0.87;
		my->y += -0.23;
		my->z += -1;

		my->pitch += PI / 2 + PI / 5;
		my->yaw -= PI / 3 - 2 * PI / 5;
		my->roll -= PI / 2;

		my->focalx = 0;
		my->focaly = -1.2;
		my->focalz = -0.1;
	}
	else if ( quiver && hudweapon && crossbow )
	{
		if ( hudweapon->skill[8] != 1 && hudweapon->skill[8] != 2 && hudweapon->skill[8] != 4 )
		{
			my->x += hudweapon->fskill[0]; // HUDWEAPON_MOVEX
			my->z += hudweapon->fskill[2]; // HUDWEAPON_MOVEZ
		}
		my->focalx = 0;
		my->focaly = 0;
		my->focalz = 0;
	}
	else
	{
		my->focalx = 0;
		my->focaly = 0;
		my->focalz = 0;
	}

	if ( my->sprite == items[TOOL_TINKERING_KIT].fpindex && !hideShield )
	{
		my->yaw += PI / 2 - HUDSHIELD_YAW / 4;
		my->focalz -= 0.5;
	}

	if ( playerRace == SPIDER && hudarm && players[clientnum]->entity->bodyparts.at(0) )
	{
		my->sprite = 854;
		my->x = hudarm->x;
		my->y = -hudarm->y;
		my->z = hudarm->z;
		my->pitch = hudarm->pitch - camera_shakey2 / 200.f;
		my->roll = -hudarm->roll;
		my->yaw = -players[clientnum]->entity->bodyparts.at(0)->yaw + players[clientnum]->entity->fskill[10] - camera_shakex2;
		my->scalex = hudarm->scalex;
		my->scaley = hudarm->scaley;
		my->scalez = hudarm->scalez;
		my->focalz = hudarm->focalz;
	}

	// dirty hack because we altered the camera height in actPlayer(). adjusts HUD to match new height.
	if ( playerRace == CREATURE_IMP && players[clientnum]->entity->z == -4.5 )
	{
		my->z -= .5;
	}
	else if ( playerRace == TROLL && players[clientnum]->entity->z <= -1.5 )
	{
		my->z -= -2 * .5;
	}


	// torch/lantern flames
	my->flags[BRIGHT] = false;
	if ( playerRace == TROLL || playerRace == SPIDER || playerRace == CREATURE_IMP || playerRace == RAT )
	{
		// don't process flames as these don't hold torches.
		return;
	}
	if (stats[clientnum]->shield && !swimming && players[clientnum]->entity->skill[3] == 0 && !cast_animation.active && !cast_animation.active_spellbook && !shieldSwitch)
	{
		if (itemCategory(stats[clientnum]->shield) == TOOL)
		{
			if (stats[clientnum]->shield->type == TOOL_TORCH)
			{
				Entity* entity = spawnFlame(my, SPRITE_FLAME);
				entity->flags[OVERDRAW] = true;
				entity->z -= 2.5 * cos(HUDSHIELD_ROLL);
				entity->y += 2.5 * sin(HUDSHIELD_ROLL);
				my->flags[BRIGHT] = true;
			}
			if ( stats[clientnum]->shield->type == TOOL_CRYSTALSHARD )
			{
				Entity* entity = spawnFlame(my, SPRITE_CRYSTALFLAME);
				entity->flags[OVERDRAW] = true;
				entity->z -= 2.5 * cos(HUDSHIELD_ROLL);
				entity->y += 2.5 * sin(HUDSHIELD_ROLL);
				my->flags[BRIGHT] = true;
			}
			else if (stats[clientnum]->shield->type == TOOL_LANTERN)
			{
				Entity* entity = spawnFlame(my, SPRITE_FLAME);
				entity->flags[OVERDRAW] = true;
				entity->z += 1;
				my->flags[BRIGHT] = true;
			}
		}
	}
}

void actHudAdditional(Entity* my)
{
	bool spellbook = false;
	if ( stats[clientnum]->shield && itemCategory(stats[clientnum]->shield) == SPELLBOOK )
	{
		spellbook = true;
	}

	my->flags[UNCLICKABLE] = true;

	// isn't active during intro/menu sequence
	if ( intro == true )
	{
		my->flags[INVISIBLE] = true;
		return;
	}

	if ( multiplayer == CLIENT )
	{
		if ( stats[clientnum]->HP <= 0 )
		{
			my->flags[INVISIBLE] = true;
			return;
		}
	}

	// this entity only exists so long as the player exists
	if ( players[clientnum] == nullptr || players[clientnum]->entity == nullptr || !hudweapon )
	{
		list_RemoveNode(my->mynode);
		return;
	}

	if ( !spellbook )
	{
		my->flags[INVISIBLE] = true;
		return;
	}

	if ( !players[clientnum]->entity->bodyparts.at(2) 
		|| players[clientnum]->entity->bodyparts.at(2)->flags[INVISIBLE]
		|| players[clientnum]->entity->bodyparts.at(2)->sprite == 854 )
	{
		// if shield invisible or spider arm we're invis.
		my->flags[INVISIBLE] = true;
		return;
	}

	if ( stats[clientnum]->shield == nullptr )
	{
		my->flags[INVISIBLE] = true;
	}
	else
	{
		if ( itemModelFirstperson(stats[clientnum]->shield) != itemModel(stats[clientnum]->shield) )
		{
			if ( spellbook )
			{
				my->scalex = 0.35;
				my->scaley = 0.35;
				my->scalez = 0.35;
			}
			else
			{
				my->scalex = 0.5f;
				my->scaley = 0.5f;
				my->scalez = 0.5f;
			}
		}
		else
		{
			my->scalex = 1.f;
			my->scaley = 1.f;
			my->scalez = 1.f;
		}
		my->sprite = itemModelFirstperson(stats[clientnum]->shield);
		my->flags[INVISIBLE] = false;
	}

	if ( cast_animation.active )
	{
		my->flags[INVISIBLE] = true;
	}
	else if ( cast_animation.active_spellbook && !spellbook )
	{
		my->flags[INVISIBLE] = true;
	}

	bool defending = false;
	bool sneaking = false;
	/*if ( !command )
	{
		if ( players[clientnum] && players[clientnum]->entity
			&& (*inputPressed(impulses[IN_DEFEND]) || (shootmode && *inputPressed(joyimpulses[INJOY_GAME_DEFEND])))
			&& players[clientnum]->entity->isMobile()
			&& !gamePaused
			&& !cast_animation.active )
		{
			if ( stats[clientnum]->shield && (hudweapon->skill[0] % 3 == 0) )
			{
				defending = true;
			}
			sneaking = true;
		}
	}*/

	// shield switching animation
	if ( shieldSwitch )
	{
		if ( spellbook )
		{
			shieldSwitch = false;
		}
		if ( !(defending || (spellbook && cast_animation.active_spellbook)) )
		{
			HUDSHIELD_MOVEY = -6;
			HUDSHIELD_MOVEZ = 2;
			HUDSHIELD_MOVEX = -2;
		}
	}

	// main animation
	if ( cast_animation.active_spellbook && spellbook )
	{
		HUDSHIELD_ROLL = std::max<real_t>(HUDSHIELD_ROLL - .1, -1 * PI / 3);
		if ( HUDSHIELD_MOVEZ > -1 )
		{
			HUDSHIELD_MOVEZ -= .2;
			if ( HUDSHIELD_MOVEZ < -1 )
			{
				HUDSHIELD_MOVEZ = -1;
			}
		}
	}
	else
	{
		if ( HUDSHIELD_MOVEX > 0 )
		{
			HUDSHIELD_MOVEX = std::max<real_t>(HUDSHIELD_MOVEX - .5, 0.0);
		}
		else if ( HUDSHIELD_MOVEX < 0 )
		{
			HUDSHIELD_MOVEX = std::min<real_t>(HUDSHIELD_MOVEX + .5, 0.0);
		}
		if ( HUDSHIELD_MOVEY > 0 )
		{
			HUDSHIELD_MOVEY = std::max<real_t>(HUDSHIELD_MOVEY - .5, 0.0);
		}
		else if ( HUDSHIELD_MOVEY < 0 )
		{
			HUDSHIELD_MOVEY = std::min<real_t>(HUDSHIELD_MOVEY + .5, 0.0);
		}
		if ( HUDSHIELD_MOVEZ > 0 )
		{
			HUDSHIELD_MOVEZ = std::max<real_t>(HUDSHIELD_MOVEZ - .2, 0.0);
		}
		else if ( HUDSHIELD_MOVEZ < 0 )
		{
			HUDSHIELD_MOVEZ = std::min<real_t>(HUDSHIELD_MOVEZ + .2, 0.0);
		}
		if ( HUDSHIELD_YAW > 0 )
		{
			HUDSHIELD_YAW = std::max<real_t>(HUDSHIELD_YAW - .15, 0.0);
		}
		else if ( HUDSHIELD_YAW < 0 )
		{
			HUDSHIELD_YAW = std::min<real_t>(HUDSHIELD_YAW + .15, 0.0);
		}
		if ( HUDSHIELD_PITCH > 0 )
		{
			HUDSHIELD_PITCH = std::max<real_t>(HUDSHIELD_PITCH - .15, 0.0);
		}
		else if ( HUDSHIELD_PITCH < 0 )
		{
			HUDSHIELD_PITCH = std::min<real_t>(HUDSHIELD_PITCH + .15, 0.0);
		}
		if ( HUDSHIELD_ROLL > 0 )
		{
			HUDSHIELD_ROLL = std::max<real_t>(HUDSHIELD_ROLL - .15, 0);
		}
		else if ( HUDSHIELD_ROLL < 0 )
		{
			if ( spellbook )
			{
				HUDSHIELD_ROLL = std::min<real_t>(HUDSHIELD_ROLL + .10, 0);
			}
			else
			{
				HUDSHIELD_ROLL = std::min<real_t>(HUDSHIELD_ROLL + .15, 0);
			}
		}
	}

	// set entity position
	my->x = 7 + HUDSHIELD_MOVEX;
	my->y = -3.5 + HUDSHIELD_MOVEY;
	my->z = 6 + HUDSHIELD_MOVEZ + (camera.z * .5 - players[clientnum]->entity->z);
	my->yaw = HUDSHIELD_YAW - camera_shakex2 - PI / 3;
	my->pitch = HUDSHIELD_PITCH - camera_shakey2 / 200.f;
	my->roll = HUDSHIELD_ROLL;
	if ( spellbook )
	{
		my->sprite += items[(stats[clientnum]->shield->type)].variations;
		my->pitch += PI / 2 + PI / 5;
		my->yaw -= PI / 3 - 2 * PI / 5;
		my->roll -= PI / 2;

		my->x += 1;
		my->y += -0.2;
		my->z -= 0.95;

		my->focalx = 0;
		my->focaly = -1.1;
		my->focalz = 0.25;
	}
	else
	{
		my->focalx = 0;
		my->focaly = 0;
		my->focalz = 0;
	}

	// dirty hack because we altered the camera height in actPlayer(). adjusts HUD to match new height.
	if ( stats[clientnum]->type == CREATURE_IMP && players[clientnum]->entity->z == -4.5 )
	{
		my->z -= .5;
	}
	else if ( stats[clientnum]->type == TROLL && players[clientnum]->entity->z <= -1.5 )
	{
		my->z -= -2 * .5;
	}
}

void actHudArrowModel(Entity* my)
{
	bool bow = false;
	if ( stats[clientnum]->weapon 
		&& (stats[clientnum]->weapon->type == SHORTBOW
			|| stats[clientnum]->weapon->type == LONGBOW
			|| stats[clientnum]->weapon->type == ARTIFACT_BOW
			|| stats[clientnum]->weapon->type == COMPOUND_BOW ) 
		)
	{
		bow = true;
	}

	my->flags[UNCLICKABLE] = true;

	// isn't active during intro/menu sequence
	if ( intro == true )
	{
		my->flags[INVISIBLE] = true;
		return;
	}

	if ( multiplayer == CLIENT )
	{
		if ( stats[clientnum]->HP <= 0 )
		{
			my->flags[INVISIBLE] = true;
			return;
		}
	}

	// this entity only exists so long as the player exists
	if ( players[clientnum] == nullptr || players[clientnum]->entity == nullptr || !hudweapon )
	{
		list_RemoveNode(my->mynode);
		return;
	}

	if ( !bow || cast_animation.active || cast_animation.active_spellbook )
	{
		my->flags[INVISIBLE] = true;
		return;
	}

	if ( hudweapon->flags[INVISIBLE] 
		|| hudweapon->skill[6] != 0
		|| hudweapon->skill[7] != 2 ) // skill[6] is hiding weapon, skill[7] is shooting something
	{
		my->flags[INVISIBLE] = true;
		return;
	}

	my->scalex = 1.f;
	my->scaley = 1.f;
	my->scalez = 1.f;
	my->flags[INVISIBLE] = false;
	my->sprite = 934;

	if ( stats[clientnum]->shield && itemTypeIsQuiver(stats[clientnum]->shield->type) )
	{
		switch ( stats[clientnum]->shield->type )
		{
			case QUIVER_SILVER:
				my->sprite = 935;
				break;
			case QUIVER_PIERCE:
				my->sprite = 936;
				break;
			case QUIVER_LIGHTWEIGHT:
				my->sprite = 937;
				break;
			case QUIVER_FIRE:
				my->sprite = 938;
				break;
			case QUIVER_HEAVY:
				my->sprite = 939;
				break;
			case QUIVER_CRYSTAL:
				my->sprite = 940;
				break;
			case QUIVER_7:
				my->sprite = 941;
				break;
			default:
				break;
		}
	}

	// set entity position
	my->x = hudweapon->x;
	my->y = hudweapon->y;
	my->z = hudweapon->z;

	my->yaw = hudweapon->yaw;
	my->pitch = hudweapon->pitch;
	my->roll = hudweapon->roll;

	my->focalx = hudweapon->focalx - 0.125;
	if ( hudweapon->sprite == items[COMPOUND_BOW].fpindex + 1 )
	{
		my->focalx += 0.125; // shorter bowstring on compound bow, push arrow forward.
	}
	my->focaly = hudweapon->focaly - 0.25;
	my->focalz = hudweapon->focalz - 0.25;

	my->scalex = hudweapon->scalex;
	my->scaley = hudweapon->scaley;
	my->scalez = hudweapon->scalez;
}