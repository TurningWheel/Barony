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

/*-------------------------------------------------------------------------------

	act*

	The following function describes an entity behavior. The function
	takes a pointer to the entity that uses it as an argument.

-------------------------------------------------------------------------------*/

#define HUDARM_PLAYERNUM my->skill[11]
#define HUD_SHAPESHIFT_HIDE my->skill[12]
#define HUD_LASTSHAPESHIFT_FORM my->skill[13]

void actHudArm(Entity* my)
{
	players[HUDARM_PLAYERNUM]->hud.arm = my;
	Entity* parent = players[HUDARM_PLAYERNUM]->hud.weapon;

	if (players[HUDARM_PLAYERNUM] == nullptr || players[HUDARM_PLAYERNUM]->entity == nullptr)
	{
		players[HUDARM_PLAYERNUM]->hud.arm = nullptr;
		list_RemoveNode(my->mynode);
		return;
	}

	if (stats[HUDARM_PLAYERNUM]->HP <= 0)
	{
		players[HUDARM_PLAYERNUM]->hud.arm = nullptr;
		list_RemoveNode(my->mynode);
		return;
	}

	if (parent == nullptr)
	{
		players[HUDARM_PLAYERNUM]->hud.arm = nullptr;
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

	Monster playerRace = players[HUDARM_PLAYERNUM]->entity->getMonsterFromPlayerRace(stats[HUDARM_PLAYERNUM]->playerRace);
	int playerAppearance = stats[HUDARM_PLAYERNUM]->appearance;
	if ( players[HUDARM_PLAYERNUM]->entity->effectShapeshift != NOTHING )
	{
		playerRace = static_cast<Monster>(players[HUDARM_PLAYERNUM]->entity->effectShapeshift);
		if ( playerRace == RAT || playerRace == SPIDER )
		{
			HUD_SHAPESHIFT_HIDE = 1;
		}
	}
	else if ( players[HUDARM_PLAYERNUM]->entity->effectPolymorph != NOTHING )
	{
		if ( players[HUDARM_PLAYERNUM]->entity->effectPolymorph > NUMMONSTERS )
		{
			playerRace = HUMAN;
			playerAppearance = players[HUDARM_PLAYERNUM]->entity->effectPolymorph - 100;
		}
		else
		{
			playerRace = static_cast<Monster>(players[HUDARM_PLAYERNUM]->entity->effectPolymorph);
		}
	}

	// when reverting form, render main limb as invisible for 1 tick as it's position needs to settle.
	if ( HUD_LASTSHAPESHIFT_FORM != playerRace )
	{
		if ( HUD_SHAPESHIFT_HIDE > 0 )
		{
			my->flags[INVISIBLE] = true;
			--HUD_SHAPESHIFT_HIDE;
		}
	}
	HUD_LASTSHAPESHIFT_FORM = playerRace;

	bool noGloves = false;
	bool hideWeapon = false;
	if (stats[HUDARM_PLAYERNUM]->gloves == nullptr
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
		if ( stats[HUDARM_PLAYERNUM]->gloves->type == GLOVES || stats[HUDARM_PLAYERNUM]->gloves->type == GLOVES_DEXTERITY )
		{
			my->sprite = 637;
		}
		else if ( stats[HUDARM_PLAYERNUM]->gloves->type == BRACERS || stats[HUDARM_PLAYERNUM]->gloves->type == BRACERS_CONSTITUTION )
		{
			my->sprite = 638;
		}
		else if ( stats[HUDARM_PLAYERNUM]->gloves->type == GAUNTLETS || stats[HUDARM_PLAYERNUM]->gloves->type == GAUNTLETS_STRENGTH )
		{
			my->sprite = 639;
		}
		else if ( stats[HUDARM_PLAYERNUM]->gloves->type == BRASS_KNUCKLES )
		{
			my->sprite = 640;
		}
		else if ( stats[HUDARM_PLAYERNUM]->gloves->type == IRON_KNUCKLES )
		{
			my->sprite = 641;
		}
		else if ( stats[HUDARM_PLAYERNUM]->gloves->type == SPIKED_GAUNTLETS )
		{
			my->sprite = 642;
		}
		else if ( stats[HUDARM_PLAYERNUM]->gloves->type == CRYSTAL_GLOVES )
		{
			my->sprite = 591;
		}
		else if ( stats[HUDARM_PLAYERNUM]->gloves->type == ARTIFACT_GLOVES )
		{
			my->sprite = 590;
		}
		else if ( stats[HUDARM_PLAYERNUM]->gloves->type == SUEDE_GLOVES )
		{
			my->sprite = 802;
		}
		if ( stats[HUDARM_PLAYERNUM]->weapon == nullptr )
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
				if ( stats[HUDARM_PLAYERNUM]->sex == FEMALE )
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

		if ( stats[HUDARM_PLAYERNUM]->weapon == nullptr || hideWeapon )
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
		my->yaw = parent->yaw + players[HUDARM_PLAYERNUM]->entity->fskill[10]; // PLAYER_SIDEBOB
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
		my->yaw = parent->yaw + players[HUDARM_PLAYERNUM]->entity->fskill[10];
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

#define HUDWEAPON_CHOP my->skill[0]
#define HUDWEAPON_INIT my->skill[1]
#define HUDWEAPON_CHARGE my->skill[3]
#define HUDWEAPON_OVERCHARGE my->skill[4]
#define HUDWEAPON_WHIP_ANGLE my->skill[5]
#define HUDWEAPON_HIDEWEAPON my->skill[6]
#define HUDWEAPON_SHOOTING_RANGED_WEAPON my->skill[7]
#define HUDWEAPON_CROSSBOW_RELOAD_ANIMATION my->skill[8]
#define HUDWEAPON_BOW_HAS_QUIVER my->skill[9]
#define HUDWEAPON_BOW_FORCE_RELOAD my->skill[10]
#define HUDWEAPON_PLAYERNUM my->skill[11]
#define HUDWEAPON_MOVEX my->fskill[0]
#define HUDWEAPON_MOVEY my->fskill[1]
#define HUDWEAPON_MOVEZ my->fskill[2]
#define HUDWEAPON_YAW my->fskill[3]
#define HUDWEAPON_PITCH my->fskill[4]
#define HUDWEAPON_ROLL my->fskill[5]
#define HUDWEAPON_OLDVIBRATEX my->fskill[6]
#define HUDWEAPON_OLDVIBRATEY my->fskill[7]
#define HUDWEAPON_OLDVIBRATEZ my->fskill[8]

enum CrossbowAnimState : int
{
	CROSSBOW_ANIM_NONE,
	CROSSBOW_ANIM_SHOOT,
	CROSSBOW_ANIM_RELOAD_START,
	CROSSBOW_ANIM_RELOAD_END,
	CROSSBOW_ANIM_SWAPPED_WEAPON,
	CROSSBOW_ANIM_HEAVY_RELOAD_MIDPOINT
};

enum RangedWeaponAnimState : int
{
	RANGED_ANIM_IDLE,
	RANGED_ANIM_BEING_DRAWN,
	RANGED_ANIM_FIRED
};

enum CrossbowHudweaponChop : int
{
	CROSSBOW_CHOP_RELOAD_START = 16,
	CROSSBOW_CHOP_RELOAD_ENDING = 17
};

void actHudWeapon(Entity* my)
{
	double result = 0;
	ItemType type;
	bool wearingring = false;

	Player::HUD_t& playerHud = players[HUDWEAPON_PLAYERNUM]->hud;

	Entity* entity;
	Entity* parent = playerHud.arm;

	auto& camera_shakex = cameravars[HUDWEAPON_PLAYERNUM].shakex;
	auto& camera_shakey = cameravars[HUDWEAPON_PLAYERNUM].shakey;
	auto& camera_shakex2 = cameravars[HUDWEAPON_PLAYERNUM].shakex2;
	auto& camera_shakey2 = cameravars[HUDWEAPON_PLAYERNUM].shakey2;

	Sint32& throwGimpTimer = playerHud.throwGimpTimer; // player cannot throw objects unless zero
	Sint32& pickaxeGimpTimer = playerHud.pickaxeGimpTimer;
	Sint32& swapWeaponGimpTimer = playerHud.swapWeaponGimpTimer; // player cannot swap weapons unless zero
	Sint32& bowGimpTimer = playerHud.bowGimpTimer;
	bool& bowFire = playerHud.bowFire;
	bool& bowIsBeingDrawn = playerHud.bowIsBeingDrawn;
	Uint32& bowStartDrawingTick = playerHud.bowStartDrawingTick;
	const Uint32& bowDrawBaseTicks = playerHud.bowDrawBaseTicks;
#ifdef SOUND
#ifdef USE_FMOD
	FMOD_CHANNEL*& bowDrawingSoundChannel = playerHud.bowDrawingSoundChannel;
	FMOD_BOOL& bowDrawingSoundPlaying = playerHud.bowDrawingSoundPlaying;
#elif defined USE_OPENAL
	OPENAL_SOUND*& bowDrawingSoundChannel = playerHud.bowDrawingSoundChannel;
	ALboolean& bowDrawingSoundPlaying = playerHud.bowDrawingSoundPlaying;
#endif
#endif

	// isn't active during intro/menu sequence
	if ( intro == true )
	{
		my->flags[INVISIBLE] = true;
		return;
	}

	if ( multiplayer == CLIENT )
	{
		if ( stats[HUDWEAPON_PLAYERNUM]->HP <= 0 )
		{
			my->flags[INVISIBLE] = true;
			return;
		}
	}

	// initialize
	if ( !HUDWEAPON_INIT )
	{
		HUDWEAPON_INIT = 1;
		playerHud.weapon = my;
		entity = newEntity(109, 1, map.entities, nullptr); // malearmright.vox
		entity->focalz = -1.5;
		entity->parent = my->getUID();
		my->parent = entity->getUID(); // just an easy way to refer to eachother, doesn't mean much
		playerHud.arm = entity;
		parent = playerHud.arm;
		entity->behavior = &actHudArm;
		entity->skill[11] = HUDWEAPON_PLAYERNUM;
		entity->flags[OVERDRAW] = true;
		entity->flags[PASSABLE] = true;
		entity->flags[NOUPDATE] = true;
	}

	if ( players[HUDWEAPON_PLAYERNUM] == nullptr || players[HUDWEAPON_PLAYERNUM]->entity == nullptr
		|| (players[HUDWEAPON_PLAYERNUM]->entity && players[HUDWEAPON_PLAYERNUM]->entity->playerCreatedDeathCam != 0) )
	{
		playerHud.weapon = nullptr; //PLAYER DED. NULLIFY THIS.
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
		//messagePlayer(HUDWEAPON_PLAYERNUM, "%d", pickaxeGimpTimer);
		--pickaxeGimpTimer;
	}
	if ( swapWeaponGimpTimer > 0 )
	{
		--swapWeaponGimpTimer;
	}
	if ( bowGimpTimer > 0 )
	{
		--bowGimpTimer;
	}

	// check levitating value
	bool levitating = isLevitating(stats[HUDWEAPON_PLAYERNUM]);

	// swimming
	if (players[HUDWEAPON_PLAYERNUM] && players[HUDWEAPON_PLAYERNUM]->entity)
	{
		if ( players[HUDWEAPON_PLAYERNUM]->movement.isPlayerSwimming() || players[HUDWEAPON_PLAYERNUM]->entity->skill[13] != 0 )  //skill[13] PLAYER_INWATER
		{
			my->flags[INVISIBLE] = true;
			if (parent)
			{
				parent->flags[INVISIBLE] = true;
			}
			return;
		}
	}

	Monster playerRace = players[HUDWEAPON_PLAYERNUM]->entity->getMonsterFromPlayerRace(stats[HUDWEAPON_PLAYERNUM]->playerRace);
	int playerAppearance = stats[HUDWEAPON_PLAYERNUM]->appearance;
	if ( players[HUDWEAPON_PLAYERNUM]->entity->effectShapeshift != NOTHING )
	{
		playerRace = static_cast<Monster>(players[HUDWEAPON_PLAYERNUM]->entity->effectShapeshift);
	}
	else if ( players[HUDWEAPON_PLAYERNUM]->entity->effectPolymorph != NOTHING )
	{
		if ( players[HUDWEAPON_PLAYERNUM]->entity->effectPolymorph > NUMMONSTERS )
		{
			playerRace = HUMAN;
			playerAppearance = players[HUDWEAPON_PLAYERNUM]->entity->effectPolymorph - 100;
		}
		else
		{
			playerRace = static_cast<Monster>(players[HUDWEAPON_PLAYERNUM]->entity->effectPolymorph);
		}
	}

	if ( playerRace == RAT || playerRace == SPIDER )
	{
		HUD_SHAPESHIFT_HIDE = std::min(2, HUD_SHAPESHIFT_HIDE + 1);
	}

	bool hideWeapon = false;
	if ( playerRace == SPIDER
		|| playerRace == RAT
		|| playerRace == CREATURE_IMP
		|| playerRace == TROLL )
	{
		hideWeapon = true;
		if ( playerRace == CREATURE_IMP && stats[HUDWEAPON_PLAYERNUM]->weapon && itemCategory(stats[HUDWEAPON_PLAYERNUM]->weapon) == MAGICSTAFF )
		{
			hideWeapon = false;
		}
	}
	else
	{
		if ( HUD_SHAPESHIFT_HIDE > 0 )
		{
			--HUD_SHAPESHIFT_HIDE;
		}
	}

	HUDWEAPON_HIDEWEAPON = hideWeapon;
	HUDWEAPON_SHOOTING_RANGED_WEAPON = RANGED_ANIM_IDLE;

	bool rangedweapon = false;
	if ( stats[HUDWEAPON_PLAYERNUM]->weapon && !hideWeapon )
	{
		if ( isRangedWeapon(*stats[HUDWEAPON_PLAYERNUM]->weapon) )
		{
			rangedweapon = true;
		}
	}

	// select model
	if ( stats[HUDWEAPON_PLAYERNUM]->ring != nullptr )
	{
		if ( stats[HUDWEAPON_PLAYERNUM]->ring->type == RING_INVISIBILITY )
		{
			wearingring = true;
		}
	}
	if ( stats[HUDWEAPON_PLAYERNUM]->cloak != nullptr )
	{
		if ( stats[HUDWEAPON_PLAYERNUM]->cloak->type == CLOAK_INVISIBILITY )
		{
			wearingring = true;
		}
	}
	if ( players[HUDWEAPON_PLAYERNUM]->entity->skill[3] == 1 || players[HUDWEAPON_PLAYERNUM]->entity->isInvisible() )   // debug cam or player invisible
	{
		my->flags[INVISIBLE] = true;
		if (parent != nullptr)
		{
			parent->flags[INVISIBLE] = true;
		}
	}
	else
	{
		if ( stats[HUDWEAPON_PLAYERNUM]->weapon == nullptr || hideWeapon )
		{
			my->flags[INVISIBLE] = true;
			if (parent != nullptr)
			{
				parent->flags[INVISIBLE] = false;
			}
		}
		else
		{
			if ( stats[HUDWEAPON_PLAYERNUM]->weapon )
			{
				if ( stats[HUDWEAPON_PLAYERNUM]->weapon->type == TOOL_WHIP )
				{
					my->scalex = 0.6;
					my->scaley = 0.6;
					my->scalez = 0.6;
				}
				else if ( stats[HUDWEAPON_PLAYERNUM]->weapon->type == TOOL_DECOY
					|| stats[HUDWEAPON_PLAYERNUM]->weapon->type == TOOL_DUMMYBOT )
				{
					my->scalex = 0.8;
					my->scaley = 0.8;
					my->scalez = 0.8;
				}
				else if ( stats[HUDWEAPON_PLAYERNUM]->weapon->type == TOOL_SENTRYBOT
					|| stats[HUDWEAPON_PLAYERNUM]->weapon->type == TOOL_SPELLBOT )
				{
					my->scalex = 0.7;
					my->scaley = 0.7;
					my->scalez = 0.7;
				}
				else if ( (stats[HUDWEAPON_PLAYERNUM]->weapon->type >= TOOL_BOMB && stats[HUDWEAPON_PLAYERNUM]->weapon->type <= TOOL_TELEPORT_BOMB)
					|| stats[HUDWEAPON_PLAYERNUM]->weapon->type == FOOD_CREAMPIE )
				{
					my->scalex = 1.f;
					my->scaley = 1.f;
					my->scalez = 1.f;
				}
				else if ( itemModelFirstperson(stats[HUDWEAPON_PLAYERNUM]->weapon) != itemModel(stats[HUDWEAPON_PLAYERNUM]->weapon) )
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
			my->sprite = itemModelFirstperson(stats[HUDWEAPON_PLAYERNUM]->weapon);

			if ( bowIsBeingDrawn && !(stats[HUDWEAPON_PLAYERNUM]->weapon && stats[HUDWEAPON_PLAYERNUM]->weapon->type == HEAVY_CROSSBOW) )
			{
				HUDWEAPON_SHOOTING_RANGED_WEAPON = RANGED_ANIM_BEING_DRAWN;
				if ( (my->ticks - bowStartDrawingTick) >= bowDrawBaseTicks / 4 )
				{
					if ( rangedweapon )
					{
						if ( stats[HUDWEAPON_PLAYERNUM]->weapon
							&& stats[HUDWEAPON_PLAYERNUM]->weapon->type != CROSSBOW )
						{
							my->sprite++;
						}
						HUDWEAPON_SHOOTING_RANGED_WEAPON = RANGED_ANIM_FIRED;
					}
				}
			}
			else if ( stats[HUDWEAPON_PLAYERNUM]->weapon && (stats[HUDWEAPON_PLAYERNUM]->weapon->type == CROSSBOW || stats[HUDWEAPON_PLAYERNUM]->weapon->type == HEAVY_CROSSBOW) )
			{
				HUDWEAPON_SHOOTING_RANGED_WEAPON = RANGED_ANIM_BEING_DRAWN;
				if ( rangedWeaponUseQuiverOnAttack(stats[HUDWEAPON_PLAYERNUM]) )
				{
					HUDWEAPON_BOW_HAS_QUIVER = 1;
				}
				else
				{
					if ( HUDWEAPON_BOW_HAS_QUIVER == 1 )
					{
						// force a reload animation.
						HUDWEAPON_CROSSBOW_RELOAD_ANIMATION = CROSSBOW_ANIM_RELOAD_START;
						HUDWEAPON_CHOP = CROSSBOW_CHOP_RELOAD_START;
						if ( stats[HUDWEAPON_PLAYERNUM]->weapon->type == HEAVY_CROSSBOW )
						{
#ifdef SOUND
							bowDrawingSoundChannel = playSound(410, 64);
#endif
						}
						HUDWEAPON_MOVEX = -1;
					}
					HUDWEAPON_BOW_HAS_QUIVER = 0;
				}
				if ( HUDWEAPON_CHOP != 0 )
				{
					if ( HUDWEAPON_CHOP == CROSSBOW_CHOP_RELOAD_START )
					{
						if ( stats[HUDWEAPON_PLAYERNUM]->weapon->type == CROSSBOW )
						{
							my->sprite = 975; // model has been "fired" and the bowstring not pulled back
						}
						else if ( stats[HUDWEAPON_PLAYERNUM]->weapon->type == HEAVY_CROSSBOW )
						{
							my->sprite = 987;
						}
					}
					else if ( HUDWEAPON_CHOP == CROSSBOW_CHOP_RELOAD_ENDING )
					{
						if ( stats[HUDWEAPON_PLAYERNUM]->weapon->type == HEAVY_CROSSBOW )
						{
							if ( HUDWEAPON_CROSSBOW_RELOAD_ANIMATION == CROSSBOW_ANIM_HEAVY_RELOAD_MIDPOINT )
							{
								my->sprite = 985;
							}
						}
					}
					HUDWEAPON_SHOOTING_RANGED_WEAPON = RANGED_ANIM_FIRED;
				}
			}

			if ( itemCategory(stats[HUDWEAPON_PLAYERNUM]->weapon) == SPELLBOOK )
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

	// when transitioning from rat/spider to normal, need a tick to make invisible while camera/limb positions settle.
	if ( HUD_SHAPESHIFT_HIDE > 0 && HUD_SHAPESHIFT_HIDE < 2 )
	{
		my->flags[INVISIBLE] = true;
		if ( parent != NULL )
		{
			parent->flags[INVISIBLE] = true;
		}
	}

	if ( cast_animation[HUDWEAPON_PLAYERNUM].active || cast_animation[HUDWEAPON_PLAYERNUM].active_spellbook )
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

	bool shootmode = players[HUDWEAPON_PLAYERNUM]->shootmode;

	bool swingweapon = false;
	if ( players[HUDWEAPON_PLAYERNUM]->entity
		&& (*inputPressedForPlayer(HUDWEAPON_PLAYERNUM, impulses[IN_ATTACK]) || (shootmode && inputs.bControllerInputPressed(HUDWEAPON_PLAYERNUM, INJOY_GAME_ATTACK)))
		&& shootmode 
		&& !gamePaused
		&& players[HUDWEAPON_PLAYERNUM]->entity->isMobile()
		&& !(*inputPressedForPlayer(HUDWEAPON_PLAYERNUM, impulses[IN_DEFEND]) && stats[HUDWEAPON_PLAYERNUM]->defending || (shootmode && inputs.bControllerInputPressed(HUDWEAPON_PLAYERNUM, INJOY_GAME_DEFEND) && stats[HUDWEAPON_PLAYERNUM]->defending))
		&& HUDWEAPON_OVERCHARGE < MAXCHARGE )
	{
		swingweapon = true;
	}
	else if ( (*inputPressedForPlayer(HUDWEAPON_PLAYERNUM, impulses[IN_ATTACK]) || (shootmode && inputs.bControllerInputPressed(HUDWEAPON_PLAYERNUM, INJOY_GAME_ATTACK))) &&
		(*inputPressedForPlayer(HUDWEAPON_PLAYERNUM, impulses[IN_DEFEND]) && stats[HUDWEAPON_PLAYERNUM]->defending || (shootmode && inputs.bControllerInputPressed(HUDWEAPON_PLAYERNUM, INJOY_GAME_DEFEND) && stats[HUDWEAPON_PLAYERNUM]->defending)) )
	{
		if ( stats[HUDWEAPON_PLAYERNUM]->shield && stats[HUDWEAPON_PLAYERNUM]->shield->type == TOOL_TINKERING_KIT )
		{
			if ( !GenericGUI[HUDWEAPON_PLAYERNUM].isGUIOpen() )
			{
				*inputPressedForPlayer(HUDWEAPON_PLAYERNUM, impulses[IN_ATTACK]) = 0;
				inputs.controllerClearInput(HUDWEAPON_PLAYERNUM, INJOY_GAME_ATTACK);
				GenericGUI[HUDWEAPON_PLAYERNUM].openGUI(GUI_TYPE_TINKERING, stats[HUDWEAPON_PLAYERNUM]->shield);
				swapWeaponGimpTimer = 20;
				return;
			}
		}
	}

	bool thrownWeapon = stats[HUDWEAPON_PLAYERNUM]->weapon
		&& (itemCategory(stats[HUDWEAPON_PLAYERNUM]->weapon) == THROWN || itemCategory(stats[HUDWEAPON_PLAYERNUM]->weapon) == GEM
			|| stats[HUDWEAPON_PLAYERNUM]->weapon->type == FOOD_CREAMPIE);
	bool castStrikeAnimation = (players[HUDWEAPON_PLAYERNUM]->entity->skill[9] == MONSTER_POSE_SPECIAL_WINDUP1);

	// weapon switch animation
	if ( players[HUDWEAPON_PLAYERNUM]->hud.weaponSwitch )
	{
		players[HUDWEAPON_PLAYERNUM]->hud.weaponSwitch = false;
		if ( !hideWeapon )
		{
			if ( stats[HUDWEAPON_PLAYERNUM]->weapon && (stats[HUDWEAPON_PLAYERNUM]->weapon->type == CROSSBOW || stats[HUDWEAPON_PLAYERNUM]->weapon->type == HEAVY_CROSSBOW) )
			{
				swingweapon = false;
				HUDWEAPON_CHARGE = 0;
				HUDWEAPON_OVERCHARGE = 0;
				HUDWEAPON_CHOP = 0;
				throwGimpTimer = std::max(throwGimpTimer, 20);
			
				HUDWEAPON_MOVEY = 0;
				HUDWEAPON_PITCH = 0;
				HUDWEAPON_YAW = -0.1;
				HUDWEAPON_MOVEZ = 0;

				if ( rangedWeaponUseQuiverOnAttack(stats[HUDWEAPON_PLAYERNUM]) )
				{
					if ( stats[HUDWEAPON_PLAYERNUM]->weapon->type == HEAVY_CROSSBOW )
					{
#ifdef SOUND
						bowDrawingSoundChannel = playSound(410, 64);
#endif
					}
					HUDWEAPON_CROSSBOW_RELOAD_ANIMATION = CROSSBOW_ANIM_SWAPPED_WEAPON;
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
				else
				{
					HUDWEAPON_CROSSBOW_RELOAD_ANIMATION = CROSSBOW_ANIM_RELOAD_START;
					HUDWEAPON_CHOP = CROSSBOW_CHOP_RELOAD_START;
					HUDWEAPON_MOVEX = -1;
					if ( stats[HUDWEAPON_PLAYERNUM]->weapon->type == HEAVY_CROSSBOW )
					{
#ifdef SOUND
						bowDrawingSoundChannel = playSound(410, 64);
#endif
					}
				}
			}
			else if ( HUDWEAPON_CHOP == CROSSBOW_CHOP_RELOAD_START || HUDWEAPON_CHOP == CROSSBOW_CHOP_RELOAD_ENDING )
			{
				// non-crossbow, if we're in these unique crossbow states then reset the chop.
				HUDWEAPON_CHOP = 0;
			}

			if ( thrownWeapon && HUDWEAPON_CHOP > 3 )
			{
				// prevent thrown weapon rapid firing.
				swingweapon = false;
				HUDWEAPON_CHARGE = 0;
				HUDWEAPON_OVERCHARGE = 0;
				HUDWEAPON_CHOP = 0;
				throwGimpTimer = std::max(throwGimpTimer, 5);
			}

			if ( rangedweapon && stats[HUDWEAPON_PLAYERNUM]->weapon
				&& stats[HUDWEAPON_PLAYERNUM]->weapon->type != CROSSBOW
				&& stats[HUDWEAPON_PLAYERNUM]->weapon->type != HEAVY_CROSSBOW
				)
			{
				HUDWEAPON_BOW_FORCE_RELOAD = 1;
				if ( HUDWEAPON_BOW_HAS_QUIVER == 1 )
				{
					// reequiped bow, force a reload.
					bowGimpTimer = std::max(bowGimpTimer, 3);
				}
			}

			if ( !HUDWEAPON_CHOP )
			{
				HUDWEAPON_MOVEZ = 2;
				HUDWEAPON_MOVEX = -.5;
				HUDWEAPON_ROLL = -PI / 2;
			}
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

	if ( HUDWEAPON_CROSSBOW_RELOAD_ANIMATION == CROSSBOW_ANIM_SWAPPED_WEAPON )
	{
		swapWeaponGimpTimer = 20;
	}

	Uint32 bowFireRate = bowDrawBaseTicks;
	bool shakeRangedWeapon = false;
	if ( rangedweapon && stats[HUDWEAPON_PLAYERNUM]->weapon
		&& stats[HUDWEAPON_PLAYERNUM]->weapon->type != CROSSBOW
		&& stats[HUDWEAPON_PLAYERNUM]->weapon->type != HEAVY_CROSSBOW
		&& !hideWeapon )
	{
		bowFireRate = bowDrawBaseTicks * (rangedAttackGetSpeedModifier(stats[HUDWEAPON_PLAYERNUM]));
		if ( rangedWeaponUseQuiverOnAttack(stats[HUDWEAPON_PLAYERNUM]) )
		{
			HUDWEAPON_BOW_HAS_QUIVER = 1;
		}
		else
		{
			if ( HUDWEAPON_BOW_HAS_QUIVER == 1 )
			{
				// unequipped quiver, force a reload.
				HUDWEAPON_BOW_FORCE_RELOAD = 1;
				bowGimpTimer = std::max(bowGimpTimer, 12);
			}
			HUDWEAPON_BOW_HAS_QUIVER = 0;
		}

		if ( (swingweapon && HUDWEAPON_CHOP != 0) || HUDWEAPON_BOW_FORCE_RELOAD == 1 )
		{
			if ( HUDWEAPON_BOW_FORCE_RELOAD == 1 )
			{
				if ( HUDWEAPON_SHOOTING_RANGED_WEAPON == RANGED_ANIM_FIRED )
				{
					HUDWEAPON_SHOOTING_RANGED_WEAPON = RANGED_ANIM_IDLE;
				}
				// don't undo swing weapon.
			}
			else
			{
				swingweapon = false;
			}
			HUDWEAPON_BOW_FORCE_RELOAD = 0;
			HUDWEAPON_CHARGE = 0;
			HUDWEAPON_OVERCHARGE = 0;
			HUDWEAPON_CHOP = 0;
			bowIsBeingDrawn = false;
		}
		else if ( rangedWeaponUseQuiverOnAttack(stats[HUDWEAPON_PLAYERNUM]) )
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
	if ( rangedweapon && bowIsBeingDrawn && !stats[HUDWEAPON_PLAYERNUM]->defending && !hideWeapon )
	{
		if ( stats[HUDWEAPON_PLAYERNUM]->weapon->type == HEAVY_CROSSBOW )
		{
			bowFireRate = bowDrawBaseTicks * (rangedAttackGetSpeedModifier(stats[HUDWEAPON_PLAYERNUM]));
		}

		if ( (my->ticks - bowStartDrawingTick) < bowFireRate )
		{
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
		bool stopSound = true;
		if ( stats[HUDWEAPON_PLAYERNUM]->weapon && stats[HUDWEAPON_PLAYERNUM]->weapon->type == HEAVY_CROSSBOW
			&& !stats[HUDWEAPON_PLAYERNUM]->defending && !hideWeapon )
		{
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

			stopSound = false;
			if ( HUDWEAPON_CROSSBOW_RELOAD_ANIMATION == CROSSBOW_ANIM_NONE )
			{
				stopSound = true;
			}
		}

		if ( stopSound )
		{
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
	}

	bool whip = stats[HUDWEAPON_PLAYERNUM]->weapon && stats[HUDWEAPON_PLAYERNUM]->weapon->type == TOOL_WHIP;

	// main animation
	if ( HUDWEAPON_CHOP == 0 )
	{
		if ( HUDWEAPON_CROSSBOW_RELOAD_ANIMATION != CROSSBOW_ANIM_SWAPPED_WEAPON )
		{
			// CROSSBOW_ANIM_SWAPPED_WEAPON requires the weapon to not be in the swapping animation to finish.
			// otherwise, the animation sets HUDWEAPON_CHOP to 0 so it has finished.
			HUDWEAPON_CROSSBOW_RELOAD_ANIMATION = CROSSBOW_ANIM_NONE;
		}
		bool ignoreAttack = false;
		if ( swingweapon && throwGimpTimer > 0 && stats[HUDWEAPON_PLAYERNUM]->weapon &&
			( stats[HUDWEAPON_PLAYERNUM]->weapon->type == CROSSBOW
				|| itemCategory(stats[HUDWEAPON_PLAYERNUM]->weapon) == POTION
				|| itemCategory(stats[HUDWEAPON_PLAYERNUM]->weapon) == GEM
				|| itemCategory(stats[HUDWEAPON_PLAYERNUM]->weapon) == THROWN
				|| stats[HUDWEAPON_PLAYERNUM]->weapon->type == FOOD_CREAMPIE )
			)
		{
			ignoreAttack = true;
		}
		else if ( swingweapon && bowGimpTimer > 0 && rangedweapon && stats[HUDWEAPON_PLAYERNUM]->weapon
			&& stats[HUDWEAPON_PLAYERNUM]->weapon->type != CROSSBOW
			&& stats[HUDWEAPON_PLAYERNUM]->weapon->type != HEAVY_CROSSBOW
			&& !hideWeapon )
		{
			ignoreAttack = true;
		}

		if ( !ignoreAttack && (swingweapon || castStrikeAnimation) )
		{
			if ( cast_animation[HUDWEAPON_PLAYERNUM].active || cast_animation[HUDWEAPON_PLAYERNUM].active_spellbook )
			{
				messagePlayer(HUDWEAPON_PLAYERNUM, language[1301]);
				spellcastingAnimationManager_deactivate(&cast_animation[HUDWEAPON_PLAYERNUM]);
			}
			if ( castStrikeAnimation )
			{
				HUDWEAPON_CHOP = 10; // special punch
			}
			else if ( stats[HUDWEAPON_PLAYERNUM]->weapon == NULL || hideWeapon )
			{
				HUDWEAPON_CHOP = 7; // punch
			}
			else
			{
				if ( conductGameChallenges[CONDUCT_BRAWLER] || achievementBrawlerMode )
				{
					if ( itemCategory(stats[HUDWEAPON_PLAYERNUM]->weapon) == WEAPON 
						|| rangedweapon 
						|| itemCategory(stats[HUDWEAPON_PLAYERNUM]->weapon) == THROWN
						|| stats[HUDWEAPON_PLAYERNUM]->weapon->type == FOOD_CREAMPIE
						|| itemCategory(stats[HUDWEAPON_PLAYERNUM]->weapon) == MAGICSTAFF
						|| itemCategory(stats[HUDWEAPON_PLAYERNUM]->weapon) == GEM
						|| stats[HUDWEAPON_PLAYERNUM]->weapon->type == TOOL_PICKAXE )
					{
						if ( achievementBrawlerMode && conductGameChallenges[CONDUCT_BRAWLER] )
						{
							messagePlayer(HUDWEAPON_PLAYERNUM, language[2997]); // prevent attack.
							return;
						}
						if ( achievementBrawlerMode )
						{
							messagePlayer(HUDWEAPON_PLAYERNUM, language[2998]); // notify no longer eligible for achievement but still atk.
						}
						conductGameChallenges[CONDUCT_BRAWLER] = 0;
					}
				}

				if ( itemCategory(stats[HUDWEAPON_PLAYERNUM]->weapon) == WEAPON || stats[HUDWEAPON_PLAYERNUM]->weapon->type == TOOL_PICKAXE )
				{
					if ( stats[HUDWEAPON_PLAYERNUM]->weapon->type == TOOL_PICKAXE )
					{
						pickaxeGimpTimer = 40;
					}
					if ( stats[HUDWEAPON_PLAYERNUM]->weapon->type == IRON_SPEAR || stats[HUDWEAPON_PLAYERNUM]->weapon->type == ARTIFACT_SPEAR )
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
						if ( stats[HUDWEAPON_PLAYERNUM]->weapon->type == SLING
							|| stats[HUDWEAPON_PLAYERNUM]->weapon->type == SHORTBOW
							|| stats[HUDWEAPON_PLAYERNUM]->weapon->type == ARTIFACT_BOW
							|| stats[HUDWEAPON_PLAYERNUM]->weapon->type == LONGBOW
							|| stats[HUDWEAPON_PLAYERNUM]->weapon->type == COMPOUND_BOW )
						{
							if ( !stats[HUDWEAPON_PLAYERNUM]->defending && !throwGimpTimer )
							{
								// bows need to be drawn back
								if ( !bowIsBeingDrawn )
								{
									if ( bowFire )
									{
										bowFire = false;

										bool artifactBowSaveAmmo = false;
										if ( stats[HUDWEAPON_PLAYERNUM]->weapon->type == ARTIFACT_BOW /*&& rangedWeaponUseQuiverOnAttack(stats[HUDWEAPON_PLAYERNUM])*/ )
										{
											real_t amount = 0.f;
											real_t percent = getArtifactWeaponEffectChance(ARTIFACT_BOW, *(stats[HUDWEAPON_PLAYERNUM]), &amount);
											if ( (rand() % 100 < static_cast<int>(percent)) )
											{
												artifactBowSaveAmmo = true;
											}
										}

										if ( artifactBowSaveAmmo )
										{
											players[HUDWEAPON_PLAYERNUM]->entity->attack(MONSTER_POSE_RANGED_SHOOT2, 0, nullptr);
										}
										else
										{
											players[HUDWEAPON_PLAYERNUM]->entity->attack(MONSTER_POSE_RANGED_SHOOT1, 0, nullptr);
										}
										HUDWEAPON_MOVEX = 3;
										throwGimpTimer = TICKS_PER_SECOND / 4;

										if ( multiplayer == CLIENT )
										{
											if ( rangedWeaponUseQuiverOnAttack(stats[HUDWEAPON_PLAYERNUM]) && !artifactBowSaveAmmo )
											{
												Item* quiver = stats[HUDWEAPON_PLAYERNUM]->shield;
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
													stats[HUDWEAPON_PLAYERNUM]->shield = NULL;
												}
											}
										}
									}
									else
									{
										bowStartDrawingTick = my->ticks;
										bowIsBeingDrawn = true;
#ifdef SOUND
										bowDrawingSoundChannel = playSound(246, 64);
#endif
									}
								}

								real_t targety = -1.0;
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
								if ( HUDWEAPON_MOVEY > targety )
								{
									HUDWEAPON_MOVEY = std::max<real_t>(HUDWEAPON_MOVEY - 1, targety);
								}
								else if ( HUDWEAPON_MOVEY < targety )
								{
									HUDWEAPON_MOVEY = std::min<real_t>(HUDWEAPON_MOVEY + 1, targety);
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
							bool doAttack = false;
							if ( throwGimpTimer == 0 )
							{
								doAttack = true;
								bool heavyCrossbow = stats[HUDWEAPON_PLAYERNUM]->weapon && stats[HUDWEAPON_PLAYERNUM]->weapon->type == HEAVY_CROSSBOW;

								if ( heavyCrossbow )
								{
									doAttack = false;
									// need to charge up.
									if ( !bowIsBeingDrawn )
									{
										if ( bowFire )
										{
											bowFire = false;
											doAttack = true;
										}
										else
										{
											bowStartDrawingTick = my->ticks;
											bowIsBeingDrawn = true;
										}
									}
									else
									{
										shakeRangedWeapon = true;
									}
								}

								if ( doAttack )
								{
									players[HUDWEAPON_PLAYERNUM]->entity->attack(MONSTER_POSE_RANGED_SHOOT1, 0, nullptr);
									HUDWEAPON_MOVEX = -4;

									// set delay before crossbow can fire again
									throwGimpTimer = 40;

									HUDWEAPON_CHOP = CROSSBOW_CHOP_RELOAD_START;
									HUDWEAPON_CROSSBOW_RELOAD_ANIMATION = CROSSBOW_ANIM_SHOOT;

									if ( heavyCrossbow )
									{
										players[HUDWEAPON_PLAYERNUM]->entity->playerStrafeVelocity = 0.3;
										players[HUDWEAPON_PLAYERNUM]->entity->playerStrafeDir = players[HUDWEAPON_PLAYERNUM]->entity->yaw + PI;
										if ( multiplayer != CLIENT )
										{
											players[HUDWEAPON_PLAYERNUM]->entity->setEffect(EFF_KNOCKBACK, true, 30, false);
										}
										if ( players[HUDWEAPON_PLAYERNUM]->entity->skill[3] == 0 )   // debug cam OFF
										{
											camera_shakex += .06;
											camera_shakey += 6;
										}
									}

									if ( multiplayer == CLIENT )
									{
										if ( rangedWeaponUseQuiverOnAttack(stats[HUDWEAPON_PLAYERNUM]) )
										{
											Item* quiver = stats[HUDWEAPON_PLAYERNUM]->shield;
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
												stats[HUDWEAPON_PLAYERNUM]->shield = NULL;
											}
										}
									}
								}
							}

							if ( !doAttack )
							{
								// reset animation - this is for weaponswitch animation code to complete while holding attack.
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
								if ( HUDWEAPON_ROLL > 0 )
								{
									HUDWEAPON_ROLL = std::max<real_t>(HUDWEAPON_ROLL - .1, 0.0);
								}
								else if ( HUDWEAPON_ROLL < 0 )
								{
									HUDWEAPON_ROLL = std::min<real_t>(HUDWEAPON_ROLL + .1, 0.0);
								}
								if ( HUDWEAPON_PITCH > 0 )
								{
									HUDWEAPON_PITCH = std::max<real_t>(HUDWEAPON_PITCH - .1, 0.0);
								}
								else if ( HUDWEAPON_PITCH < 0 )
								{
									HUDWEAPON_PITCH = std::min<real_t>(HUDWEAPON_PITCH + .1, 0.0);
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
					Item* item = stats[HUDWEAPON_PLAYERNUM]->weapon;
					if (item)
					{
						if (itemCategory(item) == SPELLBOOK)
						{
							inputs.mouseClearLeft(HUDWEAPON_PLAYERNUM);
							players[HUDWEAPON_PLAYERNUM]->entity->attack(2, 0, nullptr); // will need to add some delay to this so you can't rapid fire spells
						}
						else if ( itemIsThrowableTinkerTool(item) )
						{
							HUDWEAPON_CHOP = 13;
						}
						else if (itemCategory(item) == MAGICSTAFF)
						{
							HUDWEAPON_CHOP = 7; // magicstaffs lunge
						}
						else if ( itemCategory(item) == THROWN || itemCategory(item) == GEM || item->type == FOOD_CREAMPIE )
						{
							if ( !throwGimpTimer )
							{
								throwGimpTimer = TICKS_PER_SECOND / 2; // limits how often you can throw objects
								HUDWEAPON_CHOP = 1; // thrown normal swing
							}
						}
						else if (item->type == TOOL_LOCKPICK || item->type == TOOL_SKELETONKEY )
						{
							// keys and lockpicks
							HUDWEAPON_MOVEX = 5;
							HUDWEAPON_CHOP = 3;
							Entity* player = players[HUDWEAPON_PLAYERNUM]->entity;
							bool foundBomb = false;
							if ( stats[HUDWEAPON_PLAYERNUM]->weapon )
							{
								bool clickedOnGUI = false;

								int tmpmousex = inputs.getMouse(HUDWEAPON_PLAYERNUM, Inputs::OX);
								int tmpmousey = inputs.getMouse(HUDWEAPON_PLAYERNUM, Inputs::OY);
								// to verify splitscreen
								// pretend move the mouse to the centre of screen.
								inputs.setMouse(HUDWEAPON_PLAYERNUM, Inputs::OX, players[HUDWEAPON_PLAYERNUM]->camera_midx()); 
								inputs.setMouse(HUDWEAPON_PLAYERNUM, Inputs::OY, players[HUDWEAPON_PLAYERNUM]->camera_midy());

								Entity* clickedOn = entityClicked(&clickedOnGUI, true, HUDWEAPON_PLAYERNUM, EntityClickType::ENTITY_CLICK_USE); // using objects

								inputs.setMouse(HUDWEAPON_PLAYERNUM, Inputs::OX, tmpmousex);
								inputs.setMouse(HUDWEAPON_PLAYERNUM, Inputs::OX, tmpmousey);
								if ( clickedOn && clickedOn->behavior == &actBomb && entityDist(clickedOn, players[HUDWEAPON_PLAYERNUM]->entity) < STRIKERANGE )
								{
									// found something
									stats[HUDWEAPON_PLAYERNUM]->weapon->apply(HUDWEAPON_PLAYERNUM, clickedOn);
									foundBomb = true;
								}
							}
							if ( !foundBomb )
							{
								real_t dist = lineTrace(player, player->x, player->y, player->yaw, STRIKERANGE, 0, false);
								if ( hit.entity && stats[HUDWEAPON_PLAYERNUM]->weapon )
								{
									stats[HUDWEAPON_PLAYERNUM]->weapon->apply(HUDWEAPON_PLAYERNUM, hit.entity);
								}
								else
								{
									bool foundWall = false;
									if ( dist < STRIKERANGE 
										&& !hit.entity && hit.mapx >= 0 && hit.mapx < map.width && hit.mapy >= 0 && hit.mapy < map.height )
									{
										// arrow traps
										if ( map.tiles[OBSTACLELAYER + hit.mapy * MAPLAYERS + hit.mapx * MAPLAYERS * map.height] == 53 )
										{
											foundWall = true;
											stats[HUDWEAPON_PLAYERNUM]->weapon->applyLockpickToWall(HUDWEAPON_PLAYERNUM, hit.mapx, hit.mapy);
										}
									}
									if ( !foundWall )
									{
										messagePlayer(HUDWEAPON_PLAYERNUM, language[503], item->getName());
									}
								}
							}
						}
						else if ( item->type >= ARTIFACT_ORB_BLUE && item->type <= ARTIFACT_ORB_GREEN )
						{
							HUDWEAPON_MOVEX = 5;
							HUDWEAPON_CHOP = 3;
							Entity* player = players[HUDWEAPON_PLAYERNUM]->entity;
							lineTrace(player, player->x, player->y, player->yaw, STRIKERANGE, 0, false);
							if ( hit.entity && stats[HUDWEAPON_PLAYERNUM]->weapon )
							{
								stats[HUDWEAPON_PLAYERNUM]->weapon->apply(HUDWEAPON_PLAYERNUM, hit.entity);
							}
							else
							{
								if ( statGetINT(stats[HUDWEAPON_PLAYERNUM], player) <= 10 )
								{
									messagePlayer(HUDWEAPON_PLAYERNUM, language[2373], item->getName());
								}
								else
								{
									messagePlayer(HUDWEAPON_PLAYERNUM, language[2372], item->getName());
								}
							}
						}
						else if ( item->type == POTION_EMPTY )
						{
							if ( throwGimpTimer == 0 )
							{
								HUDWEAPON_MOVEX = 5;
								HUDWEAPON_CHOP = 3;
								Entity* player = players[HUDWEAPON_PLAYERNUM]->entity;
								lineTrace(player, player->x, player->y, player->yaw, STRIKERANGE, 0, false);
								if ( hit.entity && stats[HUDWEAPON_PLAYERNUM]->weapon )
								{
									stats[HUDWEAPON_PLAYERNUM]->weapon->apply(HUDWEAPON_PLAYERNUM, hit.entity);
								}
								else
								{
									messagePlayer(HUDWEAPON_PLAYERNUM, language[3336]);
								}
								throwGimpTimer = TICKS_PER_SECOND / 2;
							}
						}
						else if ((itemCategory(item) == POTION || itemCategory(item) == GEM || itemCategory(item) == THROWN || item->type == FOOD_CREAMPIE ) 
							&& !throwGimpTimer)
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
							players[HUDWEAPON_PLAYERNUM]->entity->attack(1, 0, nullptr);
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
									stats[HUDWEAPON_PLAYERNUM]->weapon = NULL;
								}
							}
						}
					}
				}
			}
		}
		else
		{
			if ( !stats[HUDWEAPON_PLAYERNUM]->defending )
			{
				if ( stats[HUDWEAPON_PLAYERNUM]->weapon || hideWeapon )
				{
					if ( !hideWeapon &&
						(stats[HUDWEAPON_PLAYERNUM]->weapon->type == SLING
							|| stats[HUDWEAPON_PLAYERNUM]->weapon->type == SHORTBOW
							|| stats[HUDWEAPON_PLAYERNUM]->weapon->type == ARTIFACT_BOW
							|| stats[HUDWEAPON_PLAYERNUM]->weapon->type == LONGBOW
							|| stats[HUDWEAPON_PLAYERNUM]->weapon->type == COMPOUND_BOW) )
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
					players[HUDWEAPON_PLAYERNUM]->entity->attack(1, HUDWEAPON_CHARGE, nullptr);
					if ( stats[HUDWEAPON_PLAYERNUM]->weapon
						&& (stats[HUDWEAPON_PLAYERNUM]->weapon->type == CROSSBOW || stats[HUDWEAPON_PLAYERNUM]->weapon->type == HEAVY_CROSSBOW) )
					{
						throwGimpTimer = 40; // fix for swapping weapon to crossbow while charging.
					}
					else if ( (stats[HUDWEAPON_PLAYERNUM]->weapon
						&& stats[HUDWEAPON_PLAYERNUM]->weapon->type == TOOL_PICKAXE) || whip )
					{
						if ( pickaxeGimpTimer < 20 )
						{
							pickaxeGimpTimer = 20; // fix for swapping weapon from pickaxe causing issues.
						}
					}


					if ( thrownWeapon && multiplayer == CLIENT )
					{
						Item* item = stats[HUDWEAPON_PLAYERNUM]->weapon;
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
								stats[HUDWEAPON_PLAYERNUM]->weapon = NULL;
							}
						}
					}

					HUDWEAPON_CHARGE = 0;
					HUDWEAPON_OVERCHARGE = 0;
					if (players[HUDWEAPON_PLAYERNUM]->entity->skill[3] == 0)   // debug cam OFF
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
			Item* item = stats[HUDWEAPON_PLAYERNUM]->weapon;
			if ( item && !hideWeapon )
			{
				if ( !rangedweapon 
					&& item->type != TOOL_SKELETONKEY 
					&& item->type != TOOL_LOCKPICK 
					&& itemCategory(item) != POTION 
					&& itemCategory(item) != GEM
					&& item->type != FOOD_CREAMPIE
					&& !(item->type >= ARTIFACT_ORB_BLUE && item->type <= ARTIFACT_ORB_GREEN)
					&& !(itemIsThrowableTinkerTool(item))
					&& item->type != TOOL_WHIP )
				{
					if ( stats[HUDWEAPON_PLAYERNUM]->weapon->type != TOOL_PICKAXE && itemCategory(item) != THROWN )
					{
						HUDWEAPON_CHOP = 4;
					}
					else
					{
						if ( itemCategory(item) == THROWN )
						{
							throwGimpTimer = 20;
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
			if (stats[HUDWEAPON_PLAYERNUM]->weapon && !hideWeapon )
			{
				if ( stats[HUDWEAPON_PLAYERNUM]->weapon->type == SLING
					|| stats[HUDWEAPON_PLAYERNUM]->weapon->type == SHORTBOW
					|| stats[HUDWEAPON_PLAYERNUM]->weapon->type == ARTIFACT_BOW
					|| stats[HUDWEAPON_PLAYERNUM]->weapon->type == LONGBOW
					|| stats[HUDWEAPON_PLAYERNUM]->weapon->type == COMPOUND_BOW )
				{
					if (bowFire)
					{
						players[HUDWEAPON_PLAYERNUM]->entity->attack(0, 0, nullptr);
						HUDWEAPON_MOVEX = -2;
					}
				}
			}
		}

		bool crossbow = stats[HUDWEAPON_PLAYERNUM]->weapon && (stats[HUDWEAPON_PLAYERNUM]->weapon->type == CROSSBOW || stats[HUDWEAPON_PLAYERNUM]->weapon->type == HEAVY_CROSSBOW);

		if ( stats[HUDWEAPON_PLAYERNUM]->weapon && !hideWeapon )
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
					players[HUDWEAPON_PLAYERNUM]->entity->attack(2, HUDWEAPON_CHARGE, nullptr);
					if ( stats[HUDWEAPON_PLAYERNUM]->weapon
						&& (stats[HUDWEAPON_PLAYERNUM]->weapon->type == CROSSBOW || stats[HUDWEAPON_PLAYERNUM]->weapon->type == HEAVY_CROSSBOW) )
					{
						throwGimpTimer = 40; // fix for swapping weapon to crossbow while charging.
					}
					HUDWEAPON_CHARGE = 0;
					HUDWEAPON_OVERCHARGE = 0;
					if (players[HUDWEAPON_PLAYERNUM]->entity->skill[3] == 0)   // debug cam OFF
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
			if ( stats[HUDWEAPON_PLAYERNUM]->weapon && !hideWeapon )
			{
				int weaponSkill = getWeaponSkill(stats[HUDWEAPON_PLAYERNUM]->weapon);
				if ( weaponSkill == PRO_SWORD || stats[HUDWEAPON_PLAYERNUM]->weapon->type == STEEL_HALBERD )
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
					if ( stats[HUDWEAPON_PLAYERNUM]->weapon && hideWeapon )
					{
						players[HUDWEAPON_PLAYERNUM]->entity->attack(1, HUDWEAPON_CHARGE, nullptr);
					}
					else
					{
						players[HUDWEAPON_PLAYERNUM]->entity->attack(3, HUDWEAPON_CHARGE, nullptr);
					}
					if ( stats[HUDWEAPON_PLAYERNUM]->weapon
						&& (stats[HUDWEAPON_PLAYERNUM]->weapon->type == CROSSBOW || stats[HUDWEAPON_PLAYERNUM]->weapon->type == HEAVY_CROSSBOW) )
					{
						throwGimpTimer = 40; // fix for swapping weapon to crossbow while charging.
					}
					HUDWEAPON_CHARGE = 0;
					HUDWEAPON_OVERCHARGE = 0;
					if (players[HUDWEAPON_PLAYERNUM]->entity->skill[3] == 0)   // debug cam OFF
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
				if ( stats[HUDWEAPON_PLAYERNUM]->weapon == NULL || hideWeapon )
				{
					HUDWEAPON_CHOP = 7;
				}
				else
				{
					if ( itemCategory(stats[HUDWEAPON_PLAYERNUM]->weapon) != MAGICSTAFF && stats[HUDWEAPON_PLAYERNUM]->weapon->type != CRYSTAL_SPEAR && stats[HUDWEAPON_PLAYERNUM]->weapon->type != IRON_SPEAR && stats[HUDWEAPON_PLAYERNUM]->weapon->type != ARTIFACT_SPEAR )
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
				if ( players[HUDWEAPON_PLAYERNUM]->entity->skill[3] == 0 )   // debug cam OFF
				{
					camera_shakex += .06;
					camera_shakey += 6;
				}
				players[HUDWEAPON_PLAYERNUM]->entity->attack(PLAYER_POSE_GOLEM_SMASH, MAXCHARGE, nullptr);
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
					if ( players[HUDWEAPON_PLAYERNUM]->entity->skill[3] == 0 )   // debug cam OFF
					{
						camera_shakex += .07;
					}
					Entity* player = players[HUDWEAPON_PLAYERNUM]->entity;
					if ( stats[HUDWEAPON_PLAYERNUM]->weapon && (stats[HUDWEAPON_PLAYERNUM]->weapon->isTinkeringItemWithThrownLimit())
						&& !playerCanSpawnMoreTinkeringBots(stats[HUDWEAPON_PLAYERNUM]) )
					{
						throwGimpTimer = TICKS_PER_SECOND / 2; // limits how often you can throw objects
						if ( stats[HUDWEAPON_PLAYERNUM]->PROFICIENCIES[PRO_LOCKPICKING] >= SKILL_LEVEL_LEGENDARY )
						{
							messagePlayer(HUDWEAPON_PLAYERNUM, language[3884]);
						}
						else
						{
							messagePlayer(HUDWEAPON_PLAYERNUM, language[3883]);
						}
					}
					else if ( stats[HUDWEAPON_PLAYERNUM]->weapon && player )
					{
						//lineTrace(player, player->x, player->y, player->yaw, STRIKERANGE, 0, false);
						players[HUDWEAPON_PLAYERNUM]->entity->attack(2, HUDWEAPON_CHARGE, nullptr);
						throwGimpTimer = TICKS_PER_SECOND / 2; // limits how often you can throw objects
						Item* item = stats[HUDWEAPON_PLAYERNUM]->weapon;
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
								stats[HUDWEAPON_PLAYERNUM]->weapon = NULL;
							}
						}
						if ( !stats[HUDWEAPON_PLAYERNUM]->weapon )
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
	else if ( HUDWEAPON_CHOP == CROSSBOW_CHOP_RELOAD_START )     // crossbow reload
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
				if ( HUDWEAPON_CROSSBOW_RELOAD_ANIMATION == CROSSBOW_ANIM_SHOOT
					&& stats[HUDWEAPON_PLAYERNUM]->weapon && stats[HUDWEAPON_PLAYERNUM]->weapon->type == HEAVY_CROSSBOW )
				{
#ifdef SOUND
					bowDrawingSoundChannel = playSound(410, 64);
#endif
				}

				HUDWEAPON_CROSSBOW_RELOAD_ANIMATION = CROSSBOW_ANIM_RELOAD_START;

				HUDWEAPON_MOVEZ += .15;
				real_t targetZ = 1;
				if ( !rangedWeaponUseQuiverOnAttack(stats[HUDWEAPON_PLAYERNUM]) )
				{
					targetZ = 0.5;
				}
				if ( HUDWEAPON_MOVEZ > targetZ )
				{
					if ( stats[HUDWEAPON_PLAYERNUM]->weapon && stats[HUDWEAPON_PLAYERNUM]->weapon->type == HEAVY_CROSSBOW )
					{
						HUDWEAPON_CROSSBOW_RELOAD_ANIMATION = CROSSBOW_ANIM_HEAVY_RELOAD_MIDPOINT;
					}
					else
					{
						HUDWEAPON_CROSSBOW_RELOAD_ANIMATION = CROSSBOW_ANIM_RELOAD_END;
					}
					HUDWEAPON_MOVEZ = targetZ;
				}
			}
		}
		if ( fabs(HUDWEAPON_MOVEX) < 0.01 )
		{
			HUDWEAPON_CHOP = CROSSBOW_CHOP_RELOAD_ENDING;
		}

		if ( HUDWEAPON_MOVEY > 0 )
		{
			HUDWEAPON_MOVEY = std::max<real_t>(HUDWEAPON_MOVEY - 1, 0.0);
		}
		else if ( HUDWEAPON_MOVEY < 0 )
		{
			HUDWEAPON_MOVEY = std::min<real_t>(HUDWEAPON_MOVEY + 1, 0.0);
		}
		if ( HUDWEAPON_YAW > -.1 )
		{
			HUDWEAPON_YAW = std::max<real_t>(HUDWEAPON_YAW - .1, -.1);
		}
		else if ( HUDWEAPON_YAW < -.1 )
		{
			HUDWEAPON_YAW = std::min<real_t>(HUDWEAPON_YAW + .1, -.1);
		}
		if ( HUDWEAPON_ROLL > 0 )
		{
			HUDWEAPON_ROLL = std::max<real_t>(HUDWEAPON_ROLL - .1, 0.0);
		}
		else if ( HUDWEAPON_ROLL < 0 )
		{
			HUDWEAPON_ROLL = std::min<real_t>(HUDWEAPON_ROLL + .1, 0.0);
		}
		if ( HUDWEAPON_PITCH > 0 )
		{
			HUDWEAPON_PITCH = std::max<real_t>(HUDWEAPON_PITCH - .1, 0.0);
		}
		else if ( HUDWEAPON_PITCH < 0 )
		{
			HUDWEAPON_PITCH = std::min<real_t>(HUDWEAPON_PITCH + .1, 0.0);
		}
	}
	else if ( HUDWEAPON_CHOP == CROSSBOW_CHOP_RELOAD_ENDING )     // crossbow reload
	{
		if ( HUDWEAPON_MOVEZ > 1 )
		{
			HUDWEAPON_MOVEZ = std::max<real_t>(HUDWEAPON_MOVEZ - 0.2, 1);
		}
		if ( stats[HUDWEAPON_PLAYERNUM]->weapon && stats[HUDWEAPON_PLAYERNUM]->weapon->type == HEAVY_CROSSBOW )
		{
			if ( HUDWEAPON_MOVEZ > 1 )
			{
				HUDWEAPON_MOVEZ -= .1; // just in case we're overshooting from another animation or something move faster.
			}
			else
			{
				if ( rangedWeaponUseQuiverOnAttack(stats[HUDWEAPON_PLAYERNUM]) )
				{
				// we fired a quiver shot, the crossbow will be lower here so move faster.
					HUDWEAPON_MOVEZ -= .02;
					if ( HUDWEAPON_MOVEZ < 0.5 )
					{
						HUDWEAPON_CROSSBOW_RELOAD_ANIMATION = CROSSBOW_ANIM_RELOAD_END;
					}
					else if ( HUDWEAPON_MOVEZ < 0.8 )
					{
						shakeRangedWeapon = true;
					}
				}
				else
				{
					HUDWEAPON_MOVEZ -= .01;
					if ( HUDWEAPON_MOVEZ < 0.25 )
					{
						HUDWEAPON_CROSSBOW_RELOAD_ANIMATION = CROSSBOW_ANIM_RELOAD_END;
					}
					else if ( HUDWEAPON_MOVEZ < 0.4 )
					{
						shakeRangedWeapon = true;
					}
				}
			}
		}
		else
		{
			HUDWEAPON_MOVEZ -= .1;
		}
		if ( HUDWEAPON_MOVEZ < 0 )
		{
			HUDWEAPON_MOVEZ = 0;
			HUDWEAPON_CHOP = 0;
		}

		if ( HUDWEAPON_MOVEY > 0 )
		{
			HUDWEAPON_MOVEY = std::max<real_t>(HUDWEAPON_MOVEY - 1, 0.0);
		}
		else if ( HUDWEAPON_MOVEY < 0 )
		{
			HUDWEAPON_MOVEY = std::min<real_t>(HUDWEAPON_MOVEY + 1, 0.0);
		}
		if ( HUDWEAPON_YAW > -.1 )
		{
			HUDWEAPON_YAW = std::max<real_t>(HUDWEAPON_YAW - .1, -.1);
		}
		else if ( HUDWEAPON_YAW < -.1 )
		{
			HUDWEAPON_YAW = std::min<real_t>(HUDWEAPON_YAW + .1, -.1);
		}
		if ( HUDWEAPON_ROLL > 0 )
		{
			HUDWEAPON_ROLL = std::max<real_t>(HUDWEAPON_ROLL - .1, 0.0);
		}
		else if ( HUDWEAPON_ROLL < 0 )
		{
			HUDWEAPON_ROLL = std::min<real_t>(HUDWEAPON_ROLL + .1, 0.0);
		}
		if ( HUDWEAPON_PITCH > 0 )
		{
			HUDWEAPON_PITCH = std::max<real_t>(HUDWEAPON_PITCH - .1, 0.0);
		}
		else if ( HUDWEAPON_PITCH < 0 )
		{
			HUDWEAPON_PITCH = std::min<real_t>(HUDWEAPON_PITCH + .1, 0.0);
		}
	}

	if ( HUDWEAPON_CHARGE == MAXCHARGE || castStrikeAnimation 
		|| players[HUDWEAPON_PLAYERNUM]->entity->skill[9] == MONSTER_POSE_SPECIAL_WINDUP2
		|| shakeRangedWeapon )
	{
		if ( ticks % 5 == 0 && players[HUDWEAPON_PLAYERNUM]->entity->skill[9] == MONSTER_POSE_SPECIAL_WINDUP2 )
		{
			camera_shakey += 6;
		}
		if ( ticks % 2 == 0 )
		{
			if ( shakeRangedWeapon )
			{
				HUDWEAPON_MOVEX -= HUDWEAPON_OLDVIBRATEX;
				HUDWEAPON_MOVEY -= HUDWEAPON_OLDVIBRATEY;
				HUDWEAPON_OLDVIBRATEX = (rand() % 30 - 10) / 150.f;
				HUDWEAPON_OLDVIBRATEY = (rand() % 30 - 10) / 150.f;
				HUDWEAPON_MOVEX += HUDWEAPON_OLDVIBRATEX;
				HUDWEAPON_MOVEY += HUDWEAPON_OLDVIBRATEY;
			}
			else
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
		}
		if ( (castStrikeAnimation || players[HUDWEAPON_PLAYERNUM]->entity->skill[9] == MONSTER_POSE_SPECIAL_WINDUP2)
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
	if (players[HUDWEAPON_PLAYERNUM] == nullptr || players[HUDWEAPON_PLAYERNUM]->entity == nullptr)
	{
		return;
	}
	double defaultpitch = PI / 8.f;
	if (stats[HUDWEAPON_PLAYERNUM]->weapon == nullptr || hideWeapon)
	{
		if ( playerRace == RAT )
		{
			my->x = 6 + HUDWEAPON_MOVEX / 3;
			my->y = 3;// +HUDWEAPON_MOVEY;
			my->z = (cameras[HUDWEAPON_PLAYERNUM].z * .1 - players[HUDWEAPON_PLAYERNUM]->entity->z) + 7 + HUDWEAPON_MOVEZ / 10;
			my->yaw = 0.f - camera_shakex2;
			my->pitch = defaultpitch + HUDWEAPON_PITCH - camera_shakey2 / 200.f;
			my->roll = HUDWEAPON_ROLL;

			if ( isLevitating(stats[HUDWEAPON_PLAYERNUM]) )
			{
				my->z -= 2 * .4;
			}
		}
		else if ( playerRace == SPIDER )
		{
			my->x = 6 + HUDWEAPON_MOVEX;
			my->y = 3;// +HUDWEAPON_MOVEY;
			my->z = (cameras[HUDWEAPON_PLAYERNUM].z * .5 - players[HUDWEAPON_PLAYERNUM]->entity->z) + 7 + HUDWEAPON_MOVEZ;
			my->yaw = 0.f - camera_shakex2;
			my->pitch = defaultpitch + HUDWEAPON_PITCH - camera_shakey2 / 200.f;
			my->roll = HUDWEAPON_ROLL;
		}
		else
		{
			my->x = 6 + HUDWEAPON_MOVEX;
			my->y = 3 + HUDWEAPON_MOVEY;
			my->z = (cameras[HUDWEAPON_PLAYERNUM].z * .5 - players[HUDWEAPON_PLAYERNUM]->entity->z) + 7 + HUDWEAPON_MOVEZ;
			my->yaw = HUDWEAPON_YAW - camera_shakex2;
			my->pitch = defaultpitch + HUDWEAPON_PITCH - camera_shakey2 / 200.f;
			my->roll = HUDWEAPON_ROLL;
		}

		// dirty hack because we altered the camera height in actPlayer(). adjusts HUD to match new height.
		if ( playerRace == CREATURE_IMP && players[HUDWEAPON_PLAYERNUM]->entity->z == -4.5 )
		{
			my->z -= .5;
		}
		else if ( playerRace == TROLL && players[HUDWEAPON_PLAYERNUM]->entity->z <= -1.5 )
		{
			my->z -= -2 * .5;
		}
	}
	else
	{
		Item* item = stats[HUDWEAPON_PLAYERNUM]->weapon;
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
			if ( item->type == CROSSBOW || item->type == HEAVY_CROSSBOW )
			{
				my->x = 6 + HUDWEAPON_MOVEX;
				my->y = 1.5 + HUDWEAPON_MOVEY;
				my->z = (cameras[HUDWEAPON_PLAYERNUM].z * .5 - players[HUDWEAPON_PLAYERNUM]->entity->z) + 8 + HUDWEAPON_MOVEZ;
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
				my->z = (cameras[HUDWEAPON_PLAYERNUM].z * .5 - players[HUDWEAPON_PLAYERNUM]->entity->z) + 7 + HUDWEAPON_MOVEZ;
				my->yaw = HUDWEAPON_YAW - camera_shakex2;
				my->pitch = HUDWEAPON_PITCH - camera_shakey2 / 200.f;
				my->roll = HUDWEAPON_ROLL;
			}
			else if ( item->type == TOOL_WHIP )
			{
				my->x = 6 + HUDWEAPON_MOVEX + 5;
				my->y = 3 + HUDWEAPON_MOVEY - 0.5 + 1;
				//my->flags[OVERDRAW] = false;
				my->z = (cameras[HUDWEAPON_PLAYERNUM].z * .5 - players[HUDWEAPON_PLAYERNUM]->entity->z) + 7 + HUDWEAPON_MOVEZ;
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
				my->z = (cameras[HUDWEAPON_PLAYERNUM].z * .5 - players[HUDWEAPON_PLAYERNUM]->entity->z) + 7 + HUDWEAPON_MOVEZ - 3 * (itemCategory(item) == POTION);
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
		if ( playerRace == CREATURE_IMP && players[HUDWEAPON_PLAYERNUM]->entity->z == -4.5 )
		{
			my->z -= .5;
		}
		else if ( playerRace == TROLL && players[HUDWEAPON_PLAYERNUM]->entity->z <= -1.5 )
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
#define HUDSHIELD_PLAYERNUM my->skill[2]
#define HUDSHIELD_MOVEX my->fskill[0]
#define HUDSHIELD_MOVEY my->fskill[1]
#define HUDSHIELD_MOVEZ my->fskill[2]
#define HUDSHIELD_YAW my->fskill[3]
#define HUDSHIELD_PITCH my->fskill[4]
#define HUDSHIELD_ROLL my->fskill[5]

void actHudShield(Entity* my)
{
	my->flags[UNCLICKABLE] = true;

	auto& camera_shakex = cameravars[HUDSHIELD_PLAYERNUM].shakex;
	auto& camera_shakey = cameravars[HUDSHIELD_PLAYERNUM].shakey;
	auto& camera_shakex2 = cameravars[HUDSHIELD_PLAYERNUM].shakex2;
	auto& camera_shakey2 = cameravars[HUDSHIELD_PLAYERNUM].shakey2;

	// isn't active during intro/menu sequence
	if (intro == true)
	{
		my->flags[INVISIBLE] = true;
		return;
	}

	if (multiplayer == CLIENT)
	{
		if (stats[HUDSHIELD_PLAYERNUM]->HP <= 0)
		{
			my->flags[INVISIBLE] = true;
			return;
		}
	}

	// this entity only exists so long as the player exists
	if (players[HUDSHIELD_PLAYERNUM] == nullptr || players[HUDSHIELD_PLAYERNUM]->entity == nullptr || !players[HUDSHIELD_PLAYERNUM]->hud.weapon)
	{
		list_RemoveNode(my->mynode);
		return;
	}

	// check levitating value
	bool levitating = isLevitating(stats[HUDSHIELD_PLAYERNUM]);

	// select model
	bool wearingring = false;
	if ( stats[HUDSHIELD_PLAYERNUM]->ring != nullptr )
	{
		if ( stats[HUDSHIELD_PLAYERNUM]->ring->type == RING_INVISIBILITY )
		{
			wearingring = true;
		}
	}
	if ( stats[HUDSHIELD_PLAYERNUM]->cloak != nullptr )
	{
		if ( stats[HUDSHIELD_PLAYERNUM]->cloak->type == CLOAK_INVISIBILITY )
		{
			wearingring = true;
		}
	}

	Monster playerRace = players[HUDSHIELD_PLAYERNUM]->entity->getMonsterFromPlayerRace(stats[HUDSHIELD_PLAYERNUM]->playerRace);
	if ( players[HUDSHIELD_PLAYERNUM]->entity->effectShapeshift != NOTHING )
	{
		playerRace = static_cast<Monster>(players[HUDSHIELD_PLAYERNUM]->entity->effectShapeshift);
		if ( playerRace == RAT || playerRace == SPIDER )
		{
			HUD_SHAPESHIFT_HIDE = 1;
		}
	}
	else if ( players[HUDSHIELD_PLAYERNUM]->entity->effectPolymorph != NOTHING )
	{
		if ( players[HUDSHIELD_PLAYERNUM]->entity->effectPolymorph > NUMMONSTERS )
		{
			playerRace = HUMAN;
		}
		else
		{
			playerRace = static_cast<Monster>(players[HUDSHIELD_PLAYERNUM]->entity->effectPolymorph);
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
	if ( stats[HUDSHIELD_PLAYERNUM]->shield && itemCategory(stats[HUDSHIELD_PLAYERNUM]->shield) == SPELLBOOK )
	{
		spellbook = true;
		if ( playerRace == CREATURE_IMP )
		{
			hideShield = false;
		}
	}
	else if ( stats[HUDSHIELD_PLAYERNUM]->shield && itemTypeIsQuiver(stats[HUDSHIELD_PLAYERNUM]->shield->type) )
	{
		quiver = true;
	}

	// when reverting form, render shield as invisible for 2 ticks as it's position needs to settle.
	if ( HUD_LASTSHAPESHIFT_FORM != playerRace )
	{
		if ( HUD_SHAPESHIFT_HIDE > 0 )
		{
			hideShield = true;
			--HUD_SHAPESHIFT_HIDE;
		}
	}
	HUD_LASTSHAPESHIFT_FORM = playerRace;

	if ( players[HUDSHIELD_PLAYERNUM]->entity->skill[3] == 1 || players[HUDSHIELD_PLAYERNUM]->entity->isInvisible() )   // debug cam or player invisible
	{
		my->flags[INVISIBLE] = true;
	}
	else
	{
		if (stats[HUDSHIELD_PLAYERNUM]->shield == nullptr && playerRace != SPIDER )
		{
			my->flags[INVISIBLE] = true;
		}
		else
		{
			if (stats[HUDSHIELD_PLAYERNUM]->shield)
			{
				if (itemModelFirstperson(stats[HUDSHIELD_PLAYERNUM]->shield) != itemModel(stats[HUDSHIELD_PLAYERNUM]->shield))
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
			my->sprite = itemModelFirstperson(stats[HUDSHIELD_PLAYERNUM]->shield);
			my->flags[INVISIBLE] = false;
		}
	}

	// swimming
	bool swimming = false;
	if (players[HUDSHIELD_PLAYERNUM] && players[HUDSHIELD_PLAYERNUM]->entity)
	{
		if ( players[HUDWEAPON_PLAYERNUM]->movement.isPlayerSwimming() || players[HUDSHIELD_PLAYERNUM]->entity->skill[13] != 0 ) //skill[13] PLAYER_INWATER
		{
			my->flags[INVISIBLE] = true;
			Entity* parent = uidToEntity(my->parent);
			if ( parent )
			{
				parent->flags[INVISIBLE] = true;
			}
			swimming = true;
		}
	}

	if ( hideShield )
	{
		my->flags[INVISIBLE] = true;
	}
	else if ( cast_animation[HUDSHIELD_PLAYERNUM].active )
	{
		my->flags[INVISIBLE] = true;
	}
	else if ( cast_animation[HUDSHIELD_PLAYERNUM].active_spellbook && !spellbook )
	{
		my->flags[INVISIBLE] = true;
	}

	bool defending = false;
	bool sneaking = false;
	if (!command && !swimming)
	{
		if ( players[HUDSHIELD_PLAYERNUM] && players[HUDSHIELD_PLAYERNUM]->entity 
			&& (*inputPressedForPlayer(HUDSHIELD_PLAYERNUM, impulses[IN_DEFEND]) 
				|| (players[HUDSHIELD_PLAYERNUM]->shootmode && inputs.bControllerInputPressed(HUDSHIELD_PLAYERNUM, INJOY_GAME_DEFEND)))
			&& players[HUDSHIELD_PLAYERNUM]->entity->isMobile() 
			&& !gamePaused 
			&& !cast_animation[HUDSHIELD_PLAYERNUM].active
			&& !cast_animation[HUDSHIELD_PLAYERNUM].active_spellbook
			&& (!spellbook || (spellbook && hideShield)) )
		{
			if ( stats[HUDSHIELD_PLAYERNUM]->shield && (players[HUDSHIELD_PLAYERNUM]->hud.weapon->skill[0] % 3 == 0) )
			{
				defending = true;
			}
			sneaking = true;
		}
	}

	if ( stats[HUDSHIELD_PLAYERNUM]->shield && itemTypeIsQuiver(stats[HUDSHIELD_PLAYERNUM]->shield->type) )
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
		stats[HUDSHIELD_PLAYERNUM]->defending = true;
	}
	else
	{
		stats[HUDSHIELD_PLAYERNUM]->defending = false;
	}
	if ( sneaking )
	{
		stats[HUDSHIELD_PLAYERNUM]->sneaking = true;
	}
	else
	{
		stats[HUDSHIELD_PLAYERNUM]->sneaking = false;
	}

	if (multiplayer == CLIENT)
	{
		if (HUDSHIELD_DEFEND != defending || ticks % 120 == 0)
		{
			strcpy((char*)net_packet->data, "SHLD");
			net_packet->data[4] = HUDSHIELD_PLAYERNUM;
			net_packet->data[5] = defending;
			net_packet->address.host = net_server.host;
			net_packet->address.port = net_server.port;
			net_packet->len = 6;
			sendPacketSafe(net_sock, -1, net_packet, 0);
		}
		if ( HUDSHIELD_SNEAKING != sneaking || ticks % 120 == 0 )
		{
			strcpy((char*)net_packet->data, "SNEK");
			net_packet->data[4] = HUDSHIELD_PLAYERNUM;
			net_packet->data[5] = sneaking;
			net_packet->address.host = net_server.host;
			net_packet->address.port = net_server.port;
			net_packet->len = 6;
			sendPacketSafe(net_sock, -1, net_packet, 0);
		}
	}
	HUDSHIELD_DEFEND = defending;
	HUDSHIELD_SNEAKING = sneaking;

	bool crossbow = (stats[HUDSHIELD_PLAYERNUM]->weapon && (stats[HUDSHIELD_PLAYERNUM]->weapon->type == CROSSBOW || stats[HUDSHIELD_PLAYERNUM]->weapon->type == HEAVY_CROSSBOW) );
	bool doCrossbowReloadAnimation = false;
	bool doBowReload = false;

	Entity*& hudweapon = players[HUDSHIELD_PLAYERNUM]->hud.weapon;

	// shield switching animation
	if ( players[HUDWEAPON_PLAYERNUM]->hud.shieldSwitch )
	{
		if ( hudweapon )
		{
			if ( crossbow )
			{
				if ( rangedWeaponUseQuiverOnAttack(stats[HUDSHIELD_PLAYERNUM]) )
				{
					doCrossbowReloadAnimation = true;
					if ( stats[HUDSHIELD_PLAYERNUM]->weapon->type == HEAVY_CROSSBOW )
					{
						playSound(410, 64);
					}
				}
			}
			else if ( stats[HUDSHIELD_PLAYERNUM]->weapon && isRangedWeapon(*stats[HUDSHIELD_PLAYERNUM]->weapon) )
			{
				if ( rangedWeaponUseQuiverOnAttack(stats[HUDSHIELD_PLAYERNUM]) )
				{
					hudweapon->skill[10] = 1; // HUDWEAPON_BOW_FORCE_RELOAD
					hudweapon->skill[7] = RANGED_ANIM_IDLE;
					doBowReload = true;
				}
			}
		}

		if ( !spellbook )
		{
			players[HUDWEAPON_PLAYERNUM]->hud.shieldSwitch = false;
		}
		if ( !(defending || (spellbook && cast_animation[HUDSHIELD_PLAYERNUM].active_spellbook) || doBowReload) )
		{
			HUDSHIELD_MOVEY = -6;
			HUDSHIELD_MOVEZ = 2;
			HUDSHIELD_MOVEX = -2;
		}
	}

	bool crossbowReloadAnimation = true;
	if ( hudweapon && crossbow 
		&& hudweapon->skill[8] == CROSSBOW_ANIM_SWAPPED_WEAPON && rangedWeaponUseQuiverOnAttack(stats[HUDSHIELD_PLAYERNUM]) )
	{
		// CROSSBOW_ANIM_SWAPPED_WEAPON requires the weapon to not be in the swapping animation to finish.
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
	if ( defending || cast_animation[HUDSHIELD_PLAYERNUM].active_spellbook )
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
			if ( stats[HUDSHIELD_PLAYERNUM]->shield )
			{
				if ( stats[HUDSHIELD_PLAYERNUM]->shield->type == TOOL_TORCH || stats[HUDSHIELD_PLAYERNUM]->shield->type == TOOL_CRYSTALSHARD )
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
			if ( cast_animation[HUDSHIELD_PLAYERNUM].active_spellbook )
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
	else if ( !hideShield && quiver && hudweapon && rangedWeaponUseQuiverOnAttack(stats[HUDSHIELD_PLAYERNUM])
		&& hudweapon->skill[7] != RANGED_ANIM_IDLE
		&& (!crossbow || (crossbow && crossbowReloadAnimation && hudweapon->skill[8] != CROSSBOW_ANIM_RELOAD_START)) )
	{
		// skill[7] == 1 is hudweapon bow drawing, skill[8] is the crossbow reload animation state.
		if ( hudweapon->skill[7] == RANGED_ANIM_FIRED && (!crossbow || (crossbow && hudweapon->skill[8] == CROSSBOW_ANIM_SHOOT)) )
		{
			my->flags[INVISIBLE] = true;
			HUDSHIELD_MOVEY = 0;
			HUDSHIELD_PITCH = 0;
			HUDSHIELD_YAW = 0;
			HUDSHIELD_MOVEZ = 0;
			HUDSHIELD_MOVEX = 0;

			if ( crossbow && hudweapon->skill[8] == CROSSBOW_ANIM_SHOOT )
			{
				// after shooting, lower the Z to create illusion of drawing the next arrow out of your hud.
				HUDSHIELD_MOVEZ = 2;
			}
		}

		real_t targetY = 5.05;// +limbs[HUMAN][11][0];
		real_t targetPitch = PI / 2;// +limbs[HUMAN][11][1];
		real_t targetYaw = PI / 3 - 0.1;// +limbs[HUMAN][11][2];
		real_t targetZ = -3.5;// +limbs[HUMAN][12][0];
		real_t targetX = -1.75;// +limbs[HUMAN][12][1];
		if ( stats[HUDSHIELD_PLAYERNUM]->shield->type == QUIVER_LIGHTWEIGHT )
		{
			targetZ -= 0.25; // offset a bit higher.
		}

		if ( crossbow )
		{
			targetY = 4.8 + 0.01;
			targetPitch = PI / 2;
			targetYaw = PI / 3 - 0.05;
			targetZ = -2.75;
			targetX = 2.75;

			if ( hudweapon && stats[HUDSHIELD_PLAYERNUM]->weapon->type == HEAVY_CROSSBOW )
			{
				if ( hudweapon->sprite == items[HEAVY_CROSSBOW].fpindex - 1 )
				{
					targetX += 1;
				}
				else if ( hudweapon->sprite == items[HEAVY_CROSSBOW].fpindex )
				{
					targetX += -1.5;
					if ( HUDSHIELD_MOVEX < targetX )
					{
						HUDSHIELD_MOVEX += 2.5;
						if ( HUDSHIELD_MOVEX > targetX )
						{
							HUDSHIELD_MOVEX = targetX;
						}
					}
					else if ( HUDSHIELD_MOVEX > targetX )
					{
						HUDSHIELD_MOVEX -= 2.5;
						if ( HUDSHIELD_MOVEX < targetX )
						{
							HUDSHIELD_MOVEX = targetX;
						}
					}
				}
			}

			if ( stats[HUDSHIELD_PLAYERNUM]->shield->type == QUIVER_LIGHTWEIGHT )
			{
				targetZ -= 0.25; // offset a bit higher.
			}

			if ( doCrossbowReloadAnimation )
			{
				hudweapon->skill[8] = CROSSBOW_ANIM_RELOAD_START;
				hudweapon->skill[0] = CROSSBOW_CHOP_RELOAD_START;
				hudweapon->fskill[0] = -1;
				players[HUDSHIELD_PLAYERNUM]->hud.throwGimpTimer = std::max(players[HUDSHIELD_PLAYERNUM]->hud.throwGimpTimer, 20);

				if ( fabs(hudweapon->fskill[5]) < 0.01 )
				{
					players[HUDSHIELD_PLAYERNUM]->hud.throwGimpTimer = std::max(players[HUDSHIELD_PLAYERNUM]->hud.throwGimpTimer, 20);
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
		if ( doCrossbowReloadAnimation || doBowReload )
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
	my->z = 6 + HUDSHIELD_MOVEZ + (cameras[HUDSHIELD_PLAYERNUM].z * .5 - players[HUDSHIELD_PLAYERNUM]->entity->z);
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
		if ( hudweapon->skill[8] != CROSSBOW_ANIM_SHOOT
			&& hudweapon->skill[8] != CROSSBOW_ANIM_RELOAD_START
			&& hudweapon->skill[8] != CROSSBOW_ANIM_SWAPPED_WEAPON )
		{
			// only bob the arrow if idle animation or DRAW_END animation
			my->x += hudweapon->fskill[0]; // HUDWEAPON_MOVEX
			my->y += hudweapon->fskill[1]; // HUDWEAPON_MOVEY
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

	Entity*& hudarm = players[HUDSHIELD_PLAYERNUM]->hud.arm;
	if ( playerRace == SPIDER && hudarm && players[HUDSHIELD_PLAYERNUM]->entity->bodyparts.at(0) )
	{
		my->sprite = 854;
		my->x = hudarm->x;
		my->y = -hudarm->y;
		my->z = hudarm->z;
		my->pitch = hudarm->pitch - camera_shakey2 / 200.f;
		my->roll = -hudarm->roll;
		my->yaw = -players[HUDSHIELD_PLAYERNUM]->entity->bodyparts.at(0)->yaw + players[HUDSHIELD_PLAYERNUM]->entity->fskill[10] - camera_shakex2;
		my->scalex = hudarm->scalex;
		my->scaley = hudarm->scaley;
		my->scalez = hudarm->scalez;
		my->focalz = hudarm->focalz;
	}

	// dirty hack because we altered the camera height in actPlayer(). adjusts HUD to match new height.
	if ( playerRace == CREATURE_IMP && players[HUDSHIELD_PLAYERNUM]->entity->z == -4.5 )
	{
		my->z -= .5;
	}
	else if ( playerRace == TROLL && players[HUDSHIELD_PLAYERNUM]->entity->z <= -1.5 )
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
	if (stats[HUDSHIELD_PLAYERNUM]->shield 
		&& !swimming 
		&& players[HUDSHIELD_PLAYERNUM]->entity->skill[3] == 0 
		&& !cast_animation[HUDSHIELD_PLAYERNUM].active 
		&& !cast_animation[HUDSHIELD_PLAYERNUM].active_spellbook
		&& !players[HUDWEAPON_PLAYERNUM]->hud.shieldSwitch)
	{
		if (itemCategory(stats[HUDSHIELD_PLAYERNUM]->shield) == TOOL)
		{
			if (stats[HUDSHIELD_PLAYERNUM]->shield->type == TOOL_TORCH)
			{
				Entity* entity = spawnFlame(my, SPRITE_FLAME);
				entity->flags[OVERDRAW] = true;
				entity->z -= 2.5 * cos(HUDSHIELD_ROLL);
				entity->y += 2.5 * sin(HUDSHIELD_ROLL);
				entity->skill[11] = HUDSHIELD_PLAYERNUM;
				my->flags[BRIGHT] = true;
			}
			if ( stats[HUDSHIELD_PLAYERNUM]->shield->type == TOOL_CRYSTALSHARD )
			{
				Entity* entity = spawnFlame(my, SPRITE_CRYSTALFLAME);
				entity->flags[OVERDRAW] = true;
				entity->z -= 2.5 * cos(HUDSHIELD_ROLL);
				entity->y += 2.5 * sin(HUDSHIELD_ROLL);
				entity->skill[11] = HUDSHIELD_PLAYERNUM;
				my->flags[BRIGHT] = true;
			}
			else if (stats[HUDSHIELD_PLAYERNUM]->shield->type == TOOL_LANTERN)
			{
				Entity* entity = spawnFlame(my, SPRITE_FLAME);
				entity->flags[OVERDRAW] = true;
				entity->skill[11] = HUDSHIELD_PLAYERNUM;
				entity->z += 1;
				my->flags[BRIGHT] = true;
			}
		}
	}
}

void actHudAdditional(Entity* my)
{
	bool spellbook = false;
	if ( stats[HUDSHIELD_PLAYERNUM]->shield && itemCategory(stats[HUDSHIELD_PLAYERNUM]->shield) == SPELLBOOK )
	{
		spellbook = true;
	}

	my->flags[UNCLICKABLE] = true;

	auto& camera_shakex = cameravars[HUDSHIELD_PLAYERNUM].shakex;
	auto& camera_shakey = cameravars[HUDSHIELD_PLAYERNUM].shakey;
	auto& camera_shakex2 = cameravars[HUDSHIELD_PLAYERNUM].shakex2;
	auto& camera_shakey2 = cameravars[HUDSHIELD_PLAYERNUM].shakey2;

	// isn't active during intro/menu sequence
	if ( intro == true )
	{
		my->flags[INVISIBLE] = true;
		return;
	}

	if ( multiplayer == CLIENT )
	{
		if ( stats[HUDSHIELD_PLAYERNUM]->HP <= 0 )
		{
			my->flags[INVISIBLE] = true;
			return;
		}
	}

	// this entity only exists so long as the player exists
	if ( players[HUDSHIELD_PLAYERNUM] == nullptr || players[HUDSHIELD_PLAYERNUM]->entity == nullptr || !players[HUDSHIELD_PLAYERNUM]->hud.weapon )
	{
		list_RemoveNode(my->mynode);
		return;
	}

	if ( !spellbook )
	{
		my->flags[INVISIBLE] = true;
		return;
	}

	if ( !players[HUDSHIELD_PLAYERNUM]->entity->bodyparts.at(2)
		|| players[HUDSHIELD_PLAYERNUM]->entity->bodyparts.at(2)->flags[INVISIBLE]
		|| players[HUDSHIELD_PLAYERNUM]->entity->bodyparts.at(2)->sprite == 854 )
	{
		// if shield invisible or spider arm we're invis.
		my->flags[INVISIBLE] = true;
		return;
	}

	if ( stats[HUDSHIELD_PLAYERNUM]->shield == nullptr )
	{
		my->flags[INVISIBLE] = true;
	}
	else
	{
		if ( itemModelFirstperson(stats[HUDSHIELD_PLAYERNUM]->shield) != itemModel(stats[HUDSHIELD_PLAYERNUM]->shield) )
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
		my->sprite = itemModelFirstperson(stats[HUDSHIELD_PLAYERNUM]->shield);
		my->flags[INVISIBLE] = false;
	}

	if ( cast_animation[HUDSHIELD_PLAYERNUM].active )
	{
		my->flags[INVISIBLE] = true;
	}
	else if ( cast_animation[HUDSHIELD_PLAYERNUM].active_spellbook && !spellbook )
	{
		my->flags[INVISIBLE] = true;
	}

	bool defending = false;
	bool sneaking = false;
	/*if ( !command )
	{
		if ( players[HUDSHIELD_PLAYERNUM] && players[HUDSHIELD_PLAYERNUM]->entity
			&& (*inputPressedForPlayer(HUDSHIELD_PLAYERNUM, impulses[IN_DEFEND]) || (shootmode && *inputPressed(joyimpulses[INJOY_GAME_DEFEND])))
			&& players[HUDSHIELD_PLAYERNUM]->entity->isMobile()
			&& !gamePaused
			&& !cast_animation.active )
		{
			if ( stats[HUDSHIELD_PLAYERNUM]->shield && (hudweapon->skill[0] % 3 == 0) )
			{
				defending = true;
			}
			sneaking = true;
		}
	}*/

	// shield switching animation
	if ( players[HUDWEAPON_PLAYERNUM]->hud.shieldSwitch )
	{
		if ( spellbook )
		{
			players[HUDWEAPON_PLAYERNUM]->hud.shieldSwitch = false;
		}
		if ( !(defending || (spellbook && cast_animation[HUDSHIELD_PLAYERNUM].active_spellbook)) )
		{
			HUDSHIELD_MOVEY = -6;
			HUDSHIELD_MOVEZ = 2;
			HUDSHIELD_MOVEX = -2;
		}
	}

	// main animation
	if ( cast_animation[HUDSHIELD_PLAYERNUM].active_spellbook && spellbook )
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
	my->z = 6 + HUDSHIELD_MOVEZ + (cameras[HUDSHIELD_PLAYERNUM].z * .5 - players[HUDSHIELD_PLAYERNUM]->entity->z);
	my->yaw = HUDSHIELD_YAW - camera_shakex2 - PI / 3;
	my->pitch = HUDSHIELD_PITCH - camera_shakey2 / 200.f;
	my->roll = HUDSHIELD_ROLL;
	if ( spellbook )
	{
		my->sprite += items[(stats[HUDSHIELD_PLAYERNUM]->shield->type)].variations;
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
	if ( stats[HUDSHIELD_PLAYERNUM]->type == CREATURE_IMP && players[HUDSHIELD_PLAYERNUM]->entity->z == -4.5 )
	{
		my->z -= .5;
	}
	else if ( stats[HUDSHIELD_PLAYERNUM]->type == TROLL && players[HUDSHIELD_PLAYERNUM]->entity->z <= -1.5 )
	{
		my->z -= -2 * .5;
	}
}

void actHudArrowModel(Entity* my)
{
	bool bow = false;
	bool crossbow = false;
	if ( stats[HUDSHIELD_PLAYERNUM]->weapon
		&& (stats[HUDSHIELD_PLAYERNUM]->weapon->type == SHORTBOW
			|| stats[HUDSHIELD_PLAYERNUM]->weapon->type == LONGBOW
			|| stats[HUDSHIELD_PLAYERNUM]->weapon->type == ARTIFACT_BOW
			|| stats[HUDSHIELD_PLAYERNUM]->weapon->type == COMPOUND_BOW )
		)
	{
		bow = true;
	}
	else if ( stats[HUDSHIELD_PLAYERNUM]->weapon
		&& (stats[HUDSHIELD_PLAYERNUM]->weapon->type == CROSSBOW || stats[HUDSHIELD_PLAYERNUM]->weapon->type == HEAVY_CROSSBOW) )
	{
		crossbow = true;
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
		if ( stats[HUDSHIELD_PLAYERNUM]->HP <= 0 )
		{
			my->flags[INVISIBLE] = true;
			return;
		}
	}

	Entity*& hudweapon = players[HUDSHIELD_PLAYERNUM]->hud.weapon;

	// this entity only exists so long as the player exists
	if ( players[HUDSHIELD_PLAYERNUM] == nullptr || players[HUDSHIELD_PLAYERNUM]->entity == nullptr || !hudweapon )
	{
		list_RemoveNode(my->mynode);
		return;
	}

	if ( (!crossbow && !bow) || cast_animation[HUDSHIELD_PLAYERNUM].active || cast_animation[HUDSHIELD_PLAYERNUM].active_spellbook )
	{
		my->flags[INVISIBLE] = true;
		return;
	}

	if ( crossbow )
	{
		if ( hudweapon->flags[INVISIBLE] || hudweapon->skill[6] != 0 ) // skill[6] is hideWeapon
		{
			my->flags[INVISIBLE] = true;
			return;
		}
		if ( stats[HUDSHIELD_PLAYERNUM]->shield && itemTypeIsQuiver(stats[HUDSHIELD_PLAYERNUM]->shield->type) )
		{
			my->flags[INVISIBLE] = true;
			return;
		}
		if ( stats[HUDSHIELD_PLAYERNUM]->weapon->type == CROSSBOW )
		{
			if ( hudweapon->sprite != items[CROSSBOW].fpindex )
			{
				my->flags[INVISIBLE] = true;
				return;
			}
		}
		if ( stats[HUDSHIELD_PLAYERNUM]->weapon->type == HEAVY_CROSSBOW )
		{
			if ( hudweapon->sprite == items[HEAVY_CROSSBOW].fpindex + 1 )
			{
				my->flags[INVISIBLE] = true;
				return;
			}
		}
	}
	else if ( hudweapon->flags[INVISIBLE]
		|| hudweapon->skill[6] != 0
		|| hudweapon->skill[7] != RANGED_ANIM_FIRED ) // skill[6] is hiding weapon, skill[7] is shooting something
	{
		my->flags[INVISIBLE] = true;
		return;
	}

	my->scalex = 1.f;
	my->scaley = 1.f;
	my->scalez = 1.f;

	my->flags[INVISIBLE] = false;
	my->sprite = 934;

	if ( crossbow )
	{
		my->sprite = 976;
	}
	else if ( stats[HUDSHIELD_PLAYERNUM]->shield && itemTypeIsQuiver(stats[HUDSHIELD_PLAYERNUM]->shield->type) )
	{
		switch ( stats[HUDSHIELD_PLAYERNUM]->shield->type )
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
			case QUIVER_KNOCKBACK:
				my->sprite = 939;
				break;
			case QUIVER_CRYSTAL:
				my->sprite = 940;
				break;
			case QUIVER_HUNTING:
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

	if ( crossbow )
	{
		if ( hudweapon->sprite == items[CROSSBOW].fpindex )
		{
			my->focalx += 3.5;
			my->focaly += 0.25;
			my->focalz += -0.25;
		}
		else if ( hudweapon->sprite == items[HEAVY_CROSSBOW].fpindex || hudweapon->sprite == items[HEAVY_CROSSBOW].fpindex - 1 )
		{
			my->focalx += 4.5;
			my->focaly += 0.25;
			my->focalz += -0.25;

			if ( hudweapon->sprite == items[HEAVY_CROSSBOW].fpindex )
			{
				my->focalx -= 2;
			}
		}
	}

	my->scalex = hudweapon->scalex;
	my->scaley = hudweapon->scaley;
	my->scalez = hudweapon->scalez;
}