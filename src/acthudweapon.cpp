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

	bool noGloves = false;
	if (stats[clientnum]->gloves == nullptr)
	{
		noGloves = true;
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
		if ( stats[clientnum]->appearance / 6 == 0 )
		{
			my->sprite = 634;
		}
		else if ( stats[clientnum]->appearance / 6 == 1 )
		{
			my->sprite = 635;
		}
		else
		{
			my->sprite = 636;
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

	// rotation
	//my->yaw = atan2( my->y-camera.y*16, my->x-camera.x*16 );
	my->yaw = -2 * PI / 32;
	//my->fskill[0] = sqrt( pow(my->x-camera.x*16,2) + pow(my->y-camera.y*16,2) );
	//my->pitch = atan2( my->z-camera.z*.5, my->fskill[0] );
	my->pitch = -17 * PI / 32;
	//messagePlayer(0, "my y: %f, my z: %f", my->y, my->z);
}

#ifdef USE_FMOD
FMOD_CHANNEL* bowDrawingSound = NULL;
FMOD_BOOL bowDrawingSoundPlaying = 0;
#elif defined USE_OPENAL
OPENAL_SOUND* bowDrawingSound = NULL;
ALboolean bowDrawingSoundPlaying = 0;
#else
// implement bow drawing timer via SDL_GetTicks()
bool bowDrawingSound = false;
bool bowDrawingSoundPlaying = false;
Uint32 bowDrawingStart = 0;
// based on superficial analysis of the BowDraw1V1.ogg file
Uint32 bowDrawingLength = 1030;
#endif

bool bowFire = false;

#define HUDWEAPON_CHOP my->skill[0]
#define HUDWEAPON_INIT my->skill[1]
#define HUDWEAPON_CHARGE my->skill[3]
#define HUDWEAPON_OVERCHARGE my->skill[4]
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

	if ( players[clientnum] == nullptr || players[clientnum]->entity == nullptr )
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

	bool rangedweapon = false;
	if ( stats[clientnum]->weapon )
	{
		if ( stats[clientnum]->weapon->type == SLING )
		{
			rangedweapon = true;
		}
		else if ( stats[clientnum]->weapon->type == SHORTBOW )
		{
			rangedweapon = true;
		}
		else if ( stats[clientnum]->weapon->type == CROSSBOW )
		{
			rangedweapon = true;
		}
		else if ( stats[clientnum]->weapon->type == ARTIFACT_BOW )
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
		if (stats[clientnum]->weapon == nullptr)
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
				if ( itemModelFirstperson(stats[clientnum]->weapon) != itemModel(stats[clientnum]->weapon) )
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
#ifdef SOUND
			if ( bowDrawingSoundPlaying && bowDrawingSound )
			{
				unsigned int position = 0;
				unsigned int length = 0;
#ifdef USE_OPENAL
				OPENAL_Channel_GetPosition(bowDrawingSound, &position);
				OPENAL_Sound_GetLength(sounds[246], &length);

#else
				FMOD_Channel_GetPosition(bowDrawingSound, &position, FMOD_TIMEUNIT_MS);
				FMOD_Sound_GetLength(sounds[246], &length, FMOD_TIMEUNIT_MS);
#endif
				if ( position >= length / 4 )
				{
					if ( rangedweapon )
					{
						if ( stats[clientnum]->weapon && stats[clientnum]->weapon->type != CROSSBOW )
						{
							my->sprite++;
						}
					}
				}
			}
#else
			if (bowDrawingSoundPlaying && bowDrawingSound)
			{
				unsigned int position = SDL_GetTicks() - bowDrawingStart;
				unsigned int length = bowDrawingLength;
				if ( position >= length / 4 )
				{
					if ( rangedweapon )
					{
						if ( stats[clientnum]->weapon && stats[clientnum]->weapon->type != CROSSBOW )
						{
							my->sprite++;
						}
					}
				}
			}
#endif
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

	if (cast_animation.active)
	{
		my->flags[INVISIBLE] = true;
		if (parent != NULL)
		{
			parent->flags[INVISIBLE] = true;
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

	// weapon switch animation
	if ( weaponSwitch )
	{
		weaponSwitch = false;
		if ( !HUDWEAPON_CHOP )
		{
			HUDWEAPON_MOVEZ = 2;
			HUDWEAPON_MOVEX = -.5;
			HUDWEAPON_ROLL = -PI / 2;
		}
	}

	// bow drawing sound check
#ifdef SOUND
	if ( bowDrawingSound )
	{
#ifdef USE_OPENAL
		ALboolean tempBool = bowDrawingSoundPlaying;
		OPENAL_Channel_IsPlaying(bowDrawingSound, &bowDrawingSoundPlaying);
#else
		FMOD_BOOL tempBool = bowDrawingSoundPlaying;
		FMOD_Channel_IsPlaying(bowDrawingSound, &bowDrawingSoundPlaying);
#endif
		if ( tempBool && !bowDrawingSoundPlaying )
		{
			bowFire = true;
		}
		else if ( !tempBool )
		{
			bowFire = false;
		}
	}
	else
	{
		bowDrawingSoundPlaying = 0;
		bowFire = false;
	}
#else
	if ( bowDrawingSound )
	{
		bool tempBool = bowDrawingSoundPlaying;
		bowDrawingSoundPlaying = (SDL_GetTicks() - bowDrawingStart) < bowDrawingLength;
		if ( tempBool && !bowDrawingSoundPlaying )
		{
			bowFire = true;
		}
		else if ( !tempBool )
		{
			bowFire = false;
		}
	}
	else
	{
		bowDrawingSoundPlaying = 0;
		bowFire = false;
	}
#endif

	// main animation
	if ( HUDWEAPON_CHOP == 0 )
	{
		if ( swingweapon )
		{
			if (cast_animation.active)
			{
				messagePlayer(clientnum, language[1301]);
				spellcastingAnimationManager_deactivate(&cast_animation);
			}
			if ( stats[clientnum]->weapon == NULL )
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
					else if ( rangedweapon )
					{
						if ( stats[clientnum]->weapon->type == SLING || stats[clientnum]->weapon->type == SHORTBOW || stats[clientnum]->weapon->type == ARTIFACT_BOW )
						{
							if ( !stats[clientnum]->defending && !throwGimpTimer )
							{
								// bows need to be drawn back
								if (!bowDrawingSoundPlaying)
								{
									if (bowFire)
									{
										bowFire = false;
										players[clientnum]->entity->attack(0, 0, nullptr);
										HUDWEAPON_MOVEX = 3;
										throwGimpTimer = TICKS_PER_SECOND / 4;
									}
									else
									{
#ifdef SOUND
										bowDrawingSound = playSound(246, 64);
#else
										bowDrawingSound = true;
										bowDrawingStart = SDL_GetTicks();
#endif
									}
								}
								if ( HUDWEAPON_MOVEX > 0 )
								{
									HUDWEAPON_MOVEX = std::max<real_t>(HUDWEAPON_MOVEX - 1, 0.0);
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
						else if (itemCategory(item) == MAGICSTAFF)
						{
							HUDWEAPON_CHOP = 7; // magicstaffs lunge
						}
						else if (item->type == TOOL_LOCKPICK || item->type == TOOL_SKELETONKEY )
						{
							// keys and lockpicks
							HUDWEAPON_MOVEX = 5;
							HUDWEAPON_CHOP = 3;
							Entity* player = players[clientnum]->entity;
							lineTrace(player, player->x, player->y, player->yaw, STRIKERANGE, 0, false);
							if (hit.entity  && stats[clientnum]->weapon)
							{
								stats[clientnum]->weapon->apply(clientnum, hit.entity);
							}
							else
							{
								messagePlayer(clientnum, language[503], item->getName());
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
								if ( statGetINT(stats[clientnum]) <= 10 )
								{
									messagePlayer(clientnum, language[2373], item->getName());
								}
								else
								{
									messagePlayer(clientnum, language[2372], item->getName());
								}
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
							players[clientnum]->entity->attack(0, 0, nullptr);
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
				if ( stats[clientnum]->weapon )
				{
					if ( stats[clientnum]->weapon->type == SLING || stats[clientnum]->weapon->type == SHORTBOW || stats[clientnum]->weapon->type == ARTIFACT_BOW )
					{
#ifdef SOUND
						if ( bowDrawingSoundPlaying && bowDrawingSound )
						{
#ifdef USE_OPENAL
							OPENAL_Channel_Stop(bowDrawingSound);
#else
							FMOD_Channel_Stop(bowDrawingSound);
#endif
							bowDrawingSoundPlaying = 0;
							bowDrawingSound = NULL;
						}
#else
						if ( bowDrawingSoundPlaying && bowDrawingSound )
						{
							bowDrawingSoundPlaying = false;
							bowDrawingSound = false;
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
		HUDWEAPON_MOVEX -= .35;
		if ( HUDWEAPON_MOVEX < -1 )
		{
			HUDWEAPON_MOVEX = -1;
		}
		HUDWEAPON_MOVEY -= .45;
		if ( HUDWEAPON_MOVEY < -2 )
		{
			HUDWEAPON_MOVEY = -2;
		}
		HUDWEAPON_MOVEZ -= .65;
		if (HUDWEAPON_MOVEZ < -6)
		{
			HUDWEAPON_MOVEZ = -6;
			if (HUDWEAPON_PITCH == result && HUDWEAPON_ROLL == 0 && HUDWEAPON_YAW == 0 && HUDWEAPON_MOVEX == -1 && HUDWEAPON_MOVEY == -2)
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
					if ( stats[clientnum]->weapon
						&& stats[clientnum]->weapon->type == TOOL_PICKAXE )
					{
						if ( pickaxeGimpTimer < 20 )
						{
							pickaxeGimpTimer = 20; // fix for swapping weapon from pickaxe causing issues.
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
	else if ( HUDWEAPON_CHOP == 3 )     // return from first swing
	{
		if ( swingweapon )
		{
			// another swing...
			Item* item = stats[clientnum]->weapon;
			if ( item )
			{
				if ( !rangedweapon 
					&& item->type != TOOL_SKELETONKEY 
					&& item->type != TOOL_LOCKPICK 
					&& itemCategory(item) != POTION 
					&& itemCategory(item) != GEM 
					&& itemCategory(item) != THROWN
					&& !(item->type >= ARTIFACT_ORB_BLUE && item->type <= ARTIFACT_ORB_GREEN) )
				{
					if ( stats[clientnum]->weapon->type != TOOL_PICKAXE )
					{
						HUDWEAPON_CHOP = 4;
					}
					else
					{
						if ( pickaxeGimpTimer <= 0 )
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
			if (stats[clientnum]->weapon)
			{
				if (stats[clientnum]->weapon->type == SLING || stats[clientnum]->weapon->type == SHORTBOW || stats[clientnum]->weapon->type == ARTIFACT_BOW)
				{
					if (bowFire)
					{
						players[clientnum]->entity->attack(0, 0, nullptr);
						HUDWEAPON_MOVEX = -2;
					}
				}
			}
		}

		if ( stats[clientnum]->weapon != NULL )
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
		HUDWEAPON_YAW = 0;
		HUDWEAPON_PITCH -= .25;
		if ( HUDWEAPON_PITCH < 0 )
		{
			HUDWEAPON_PITCH = 0;
		}
		HUDWEAPON_MOVEX -= .35;
		if ( HUDWEAPON_MOVEX < 0 )
		{
			HUDWEAPON_MOVEX = 0;
		}
		HUDWEAPON_MOVEZ -= .75;
		if ( HUDWEAPON_MOVEZ < -4 )
		{
			HUDWEAPON_MOVEZ = -4;
		}
		HUDWEAPON_MOVEY -= .75;
		if ( HUDWEAPON_MOVEY < -6 )
		{
			HUDWEAPON_MOVEY = -6;
		}
		HUDWEAPON_ROLL -= .25;
		if (HUDWEAPON_ROLL < -PI / 2)
		{
			HUDWEAPON_ROLL = -PI / 2;
			if (HUDWEAPON_PITCH == 0 && HUDWEAPON_MOVEX == 0 && HUDWEAPON_MOVEY == -6 && HUDWEAPON_MOVEZ == -4)
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
		HUDWEAPON_MOVEX = sin(HUDWEAPON_YAW) * 1;
		HUDWEAPON_MOVEY = cos(HUDWEAPON_YAW) * -6;
		HUDWEAPON_YAW += .35;
		if (HUDWEAPON_YAW > (3 * PI) / 4)
		{
			HUDWEAPON_YAW = (3 * PI) / 4;
			HUDWEAPON_CHOP++;
		}
	}
	else if (HUDWEAPON_CHOP == 6)     // return from second swing
	{
		if ( swingweapon )
		{
			// one more swing...
			if ( stats[clientnum]->weapon )
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
		HUDWEAPON_MOVEZ += .35;
		if ( HUDWEAPON_MOVEZ > 0 )
		{
			HUDWEAPON_MOVEZ = 0;
		}
		HUDWEAPON_YAW -= .25;
		if ( HUDWEAPON_YAW < -.1 )
		{
			HUDWEAPON_YAW = -.1;
		}
		HUDWEAPON_ROLL += .25;
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
		HUDWEAPON_PITCH -= .05;
		if ( HUDWEAPON_PITCH < .2)
		{
			HUDWEAPON_PITCH = .2;
		}
		HUDWEAPON_ROLL -= .15;
		if (HUDWEAPON_ROLL < -2 * PI / 5)
		{
			HUDWEAPON_ROLL = -2 * PI / 5;
			if (HUDWEAPON_PITCH == .2 && HUDWEAPON_YAW == result && HUDWEAPON_MOVEX == 0 && HUDWEAPON_MOVEY == -1 && HUDWEAPON_MOVEZ == -2)
			{
				if (!swingweapon)
				{
					HUDWEAPON_CHOP++;
					players[clientnum]->entity->attack(3, HUDWEAPON_CHARGE, nullptr);
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
		HUDWEAPON_MOVEX += 2;
		if ( HUDWEAPON_MOVEX > 4 )
		{
			HUDWEAPON_MOVEX = 4;
			HUDWEAPON_CHOP++;
		}
	}
	else if ( HUDWEAPON_CHOP == 9 )     // return from third swing
	{
		HUDWEAPON_MOVEX -= .5;
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
				if ( stats[clientnum]->weapon == NULL )
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
			HUDWEAPON_PITCH += .05;
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

	if ( HUDWEAPON_CHARGE == MAXCHARGE )
	{
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
		HUDWEAPON_OVERCHARGE++;
	}

	// move the weapon
	if (players[clientnum] == nullptr || players[clientnum]->entity == nullptr)
	{
		return;
	}
	double defaultpitch = PI / 8.f;
	if (stats[clientnum]->weapon == nullptr)
	{
		my->x = 6 + HUDWEAPON_MOVEX;
		my->y = 3 + HUDWEAPON_MOVEY;
		my->z = (camera.z * .5 - players[clientnum]->entity->z) + 7 + HUDWEAPON_MOVEZ;
		my->yaw = HUDWEAPON_YAW - camera_shakex2;
		my->pitch = defaultpitch + HUDWEAPON_PITCH - camera_shakey2 / 200.f;
		my->roll = HUDWEAPON_ROLL;
	}
	else
	{
		Item* item = stats[clientnum]->weapon;
		if (item)
		{
			if (item->type == TOOL_SKELETONKEY || item->type == TOOL_LOCKPICK
				|| (item->type >= ARTIFACT_ORB_BLUE && item->type <= ARTIFACT_ORB_GREEN) )
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
			else if (item->type == SLING || item->type == SHORTBOW || item->type == ARTIFACT_BOW)
			{
				my->x = 6 + HUDWEAPON_MOVEX;
				my->y = 3 + HUDWEAPON_MOVEY;
				my->z = (camera.z * .5 - players[clientnum]->entity->z) + 7 + HUDWEAPON_MOVEZ;
				my->yaw = HUDWEAPON_YAW - camera_shakex2;
				my->pitch = HUDWEAPON_PITCH - camera_shakey2 / 200.f;
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
			}
		}
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
	if ( players[clientnum]->entity->skill[3] == 1 || players[clientnum]->entity->isInvisible() )   // debug cam or player invisible
	{
		my->flags[INVISIBLE] = true;
	}
	else
	{
		if (stats[clientnum]->shield == nullptr)
		{
			my->flags[INVISIBLE] = true;
		}
		else
		{
			if (stats[clientnum]->shield)
			{
				if (itemModelFirstperson(stats[clientnum]->shield) != itemModel(stats[clientnum]->shield))
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
			my->sprite = itemModelFirstperson(stats[clientnum]->shield);
			my->flags[INVISIBLE] = false;
		}
	}

	// swimming
	bool swimming = false;
	if (players[clientnum] && players[clientnum]->entity)
	{
		if (!levitating && !waterwalkingboots) //TODO: Swimming capstone?
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

	if (cast_animation.active)
	{
		my->flags[INVISIBLE] = true;
	}

	bool defending = false;
	bool sneaking = false;
	if (!command && !swimming)
	{
		if (players[clientnum] && players[clientnum]->entity 
			&& (*inputPressed(impulses[IN_DEFEND]) || (shootmode && *inputPressed(joyimpulses[INJOY_GAME_DEFEND]))) 
			&& players[clientnum]->entity->isMobile() 
			&& !gamePaused 
			&& !cast_animation.active)
		{
			if ( stats[clientnum]->shield && (hudweapon->skill[0] % 3 == 0) )
			{
				defending = true;
			}
			sneaking = true;
		}
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

	// shield switching animation
	if ( shieldSwitch )
	{
		shieldSwitch = false;
		if ( !defending )
		{
			HUDSHIELD_MOVEY = -6;
			HUDSHIELD_MOVEZ = 2;
			HUDSHIELD_MOVEX = -2;
		}
	}

	// main animation
	if ( defending )
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
	my->yaw = HUDSHIELD_YAW - camera_shakex2 - PI / 3;
	my->pitch = HUDSHIELD_PITCH - camera_shakey2 / 200.f;
	my->roll = HUDSHIELD_ROLL;

	// torch/lantern flames
	my->flags[BRIGHT] = false;
	if (stats[clientnum]->shield && !swimming && players[clientnum]->entity->skill[3] == 0 && !cast_animation.active && !shieldSwitch)
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
