/*-------------------------------------------------------------------------------

	BARONY
	File: actplayer.cpp
	Desc: behavior function(s) for player

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "messages.hpp"
#include "entity.hpp"
#include "interface/interface.hpp"
#include "sound.hpp"
#include "items.hpp"
#include "magic/magic.hpp"
#include "menu.hpp"
#include "scores.hpp"
#include "monster.hpp"
#include "net.hpp"
#include "collision.hpp"
#include "player.hpp"
#include "colors.hpp"
#include "draw.hpp"

bool smoothmouse = false;
bool settings_smoothmouse = false;
bool swimDebuffMessageHasPlayed = false;
int monsterEmoteGimpTimer = 0;

/*-------------------------------------------------------------------------------

	act*

	The following functions describe various entity behaviors. All functions
	take a pointer to the entities that use them as an argument.

-------------------------------------------------------------------------------*/

#define DEATHCAM_TIME my->skill[0]
#define DEATHCAM_PLAYER my->skill[1]
#define DEATHCAM_ROTX my->fskill[0]
#define DEATHCAM_ROTY my->fskill[1]

void actDeathCam(Entity* my)
{
	DEATHCAM_TIME++;
	if ( DEATHCAM_TIME == 1 )
	{
		DEATHCAM_PLAYER = -1;
	}
	else if ( DEATHCAM_TIME == TICKS_PER_SECOND * 6 )
	{
		openGameoverWindow();
	}
	if ( shootmode && !gamePaused )
	{
		if ( smoothmouse )
		{
			DEATHCAM_ROTX += mousexrel * .006 * (mousespeed / 128.f);
			DEATHCAM_ROTX = fmin(fmax(-0.35, DEATHCAM_ROTX), 0.35);
		}
		else
		{
			DEATHCAM_ROTX = std::min<float>(std::max<float>(-0.35f, mousexrel * .01f * (mousespeed / 128.f)), 0.35f);
		}
		my->yaw += DEATHCAM_ROTX;
		if ( my->yaw >= PI * 2 )
		{
			my->yaw -= PI * 2;
		}
		else if ( my->yaw < 0 )
		{
			my->yaw += PI * 2;
		}

		if ( smoothmouse )
		{
			DEATHCAM_ROTY += mouseyrel * .006 * (mousespeed / 128.f) * (reversemouse * 2 - 1);
			DEATHCAM_ROTY = fmin(fmax(-0.35, DEATHCAM_ROTY), 0.35);
		}
		else
		{
			DEATHCAM_ROTY = std::min<float>(std::max<float>(-0.35f, mouseyrel * .01f * (mousespeed / 128.f) * (reversemouse * 2 - 1)), 0.35f);
		}
		my->pitch -= DEATHCAM_ROTY;
		if ( my->pitch > PI / 2 )
		{
			my->pitch = PI / 2;
		}
		else if ( my->pitch < -PI / 2 )
		{
			my->pitch = -PI / 2;
		}
	}
	if ( smoothmouse )
	{
		DEATHCAM_ROTX *= .5;
		DEATHCAM_ROTY *= .5;
	}
	else
	{
		DEATHCAM_ROTX = 0;
		DEATHCAM_ROTY = 0;
	}

	if ((*inputPressed(impulses[IN_ATTACK]) || (shootmode && *inputPressed(joyimpulses[INJOY_GAME_ATTACK]))) && shootmode)
	{
		*inputPressed(impulses[IN_ATTACK]) = 0;
		if ( shootmode )
		{
			*inputPressed(joyimpulses[INJOY_GAME_ATTACK]) = 0;
		}
		DEATHCAM_PLAYER++;
		if (DEATHCAM_PLAYER >= MAXPLAYERS)
		{
			DEATHCAM_PLAYER = 0;
		}
		int c = 0;
		while (!players[DEATHCAM_PLAYER] || !players[DEATHCAM_PLAYER]->entity)   //TODO: PLAYERSWAP VERIFY. I'm not sure if the loop's condition should look like this. I think it should be fine...
		{
			if (c > MAXPLAYERS)
			{
				break;
			}
			DEATHCAM_PLAYER++;
			if (DEATHCAM_PLAYER >= MAXPLAYERS)
			{
				DEATHCAM_PLAYER = 0;
			}
			c++;
		}
	}

	if (DEATHCAM_PLAYER >= 0)
	{
		if (players[DEATHCAM_PLAYER] && players[DEATHCAM_PLAYER]->entity)
		{
			my->x = players[DEATHCAM_PLAYER]->entity->x;
			my->y = players[DEATHCAM_PLAYER]->entity->y;
		}
	}

	my->removeLightField();
	my->light = lightSphereShadow(my->x / 16, my->y / 16, 3, 128);

	camera.x = my->x / 16.f;
	camera.y = my->y / 16.f;
	camera.z = my->z * 2.f;
	camera.ang = my->yaw;
	camera.vang = my->pitch;

	camera.x -= cos(my->yaw) * cos(my->pitch) * 1.5;
	camera.y -= sin(my->yaw) * cos(my->pitch) * 1.5;
	camera.z -= sin(my->pitch) * 16;
}

#define PLAYER_INIT my->skill[0]
#define PLAYER_TORCH my->skill[1]
#define PLAYER_NUM my->skill[2]
#define PLAYER_DEBUGCAM my->skill[3]
#define PLAYER_BOBMODE my->skill[4]
#define PLAYER_ATTACK my->skill[9]
#define PLAYER_ATTACKTIME my->skill[10]
#define PLAYER_ARMBENDED my->skill[11]
#define PLAYER_ALIVETIME my->skill[12]
#define PLAYER_INWATER my->skill[13]
#define PLAYER_CLICKED my->skill[14]
#define PLAYER_VELX my->vel_x
#define PLAYER_VELY my->vel_y
#define PLAYER_VELZ my->vel_z
#define PLAYER_BOB my->fskill[0]
#define PLAYER_BOBMOVE my->fskill[1]
#define PLAYER_WEAPONYAW my->fskill[2]
#define PLAYER_DX my->fskill[3]
#define PLAYER_DY my->fskill[4]
#define PLAYER_DYAW my->fskill[5]
#define PLAYER_ROTX my->fskill[6]
#define PLAYER_ROTY my->fskill[7]
#define PLAYER_SHIELDYAW my->fskill[8]
#define PLAYERWALKSPEED .12

void actPlayer(Entity* my)
{
	if (!my)
	{
		return;
	}
	if ( logCheckObstacle )
	{
		if ( ticks % 50 == 0 )
		{
			messagePlayer(0, "checkObstacle() calls/sec: %d", logCheckObstacleCount);
			logCheckObstacleCount = 0;
		}
	}
	if ( spamming )
	{
		for (int i = 0; i < 1; ++i)
		{
			char s[64] = "";
			char alphanum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

			for ( int j = 0; j < 63; ++j ) {
				s[j] = alphanum[rand() % (sizeof(alphanum) - 1)];
			}
			Uint32 totalSize = 0;
			for ( size_t c = 0; c < HASH_SIZE; ++c ) {
				totalSize += list_Size(&ttfTextHash[c]);
			}
			messagePlayer(0, "IMGREF: %d, total size: %d", imgref, totalSize);
			s[63] = '\0';
			messagePlayer(0, "%s", s);
			//messagePlayer(0, "Lorem ipsum dolor sit amet, dico accusam reprehendunt ne mea, ea est illum tincidunt voluptatibus. Ne labore voluptua eos, nostro fierent mnesarchum an mei, cu mea dolor verear epicuri. Est id iriure principes, unum cotidieque qui te. An sit tractatos complectitur.");
		}
	}

	Entity* entity;
	Entity* entity2 = nullptr;
	Entity* rightbody = nullptr;
	Entity* weaponarm = nullptr;
	Entity* shieldarm = nullptr;
	Entity* additionalLimb = nullptr;
	Entity* torso = nullptr;
	node_t* node;
	Item* item;
	int i, bodypart;
	double dist = 0;
	double weightratio;
	int weight;
	bool wearingring = false;
	bool levitating = false;

	if ( PLAYER_NUM < 0 || PLAYER_NUM >= MAXPLAYERS )
	{
		return;
	}

	Monster playerRace = HUMAN;
	int spriteTorso = 106 + 12 * stats[PLAYER_NUM]->sex;
	int spriteLegRight = 107 + 12 * stats[PLAYER_NUM]->sex;
	int spriteLegLeft = 108 + 12 * stats[PLAYER_NUM]->sex;
	int spriteArmRight = 109 + 12 * stats[PLAYER_NUM]->sex;
	int spriteArmLeft = 110 + 12 * stats[PLAYER_NUM]->sex;
	int playerAppearance = stats[PLAYER_NUM]->appearance;

	if ( stats[PLAYER_NUM]->playerRace > 0 || stats[PLAYER_NUM]->EFFECTS[EFF_POLYMORPH] || my->effectPolymorph != NOTHING )
	{
		playerRace = my->getMonsterFromPlayerRace(stats[PLAYER_NUM]->playerRace);
		if ( my->effectPolymorph != NOTHING )
		{
			if ( my->effectPolymorph > NUMMONSTERS )
			{
				playerRace = HUMAN;
				playerAppearance = my->effectPolymorph - 100;
			}
			else
			{
				playerRace = static_cast<Monster>(my->effectPolymorph);
			}
		}
		if ( stats[PLAYER_NUM]->appearance == 0 || my->effectPolymorph != NOTHING )
		{
			stats[PLAYER_NUM]->type = playerRace;
		}
		else
		{
			stats[PLAYER_NUM]->type = HUMAN; // appearance of 1 is aesthetic only
		}
	}
	else
	{
		stats[PLAYER_NUM]->type = HUMAN;
	}

	if ( multiplayer != CLIENT )
	{
		if ( stats[PLAYER_NUM]->EFFECTS[EFF_POLYMORPH] )
		{
			stats[PLAYER_NUM]->playerPolymorphStorage = my->effectPolymorph; // keep track of player polymorph effects
		}
		else
		{
			if ( my->effectPolymorph != NOTHING ) // just in case this was cleared other than normal progression ticking down
			{
				my->effectPolymorph = NOTHING;
				serverUpdateEntitySkill(my, 50);
			}
		}
	}

	my->focalx = limbs[playerRace][0][0];
	my->focaly = limbs[playerRace][0][1];
	my->focalz = limbs[playerRace][0][2];

	if ( playerRace == GOATMAN && my->sprite == 768 )
	{
		my->focalz = limbs[playerRace][0][2] - 0.25; // minor head position fix to match male variant.
	}

	if ( multiplayer == CLIENT )
	{
		if ( PLAYER_NUM != clientnum )
		{
			my->flags[UPDATENEEDED] = true;
		}

		my->handleEffectsClient();

		// request entity update (check if I've been deleted)
		if ( ticks % (TICKS_PER_SECOND * 5) == my->getUID() % (TICKS_PER_SECOND * 5) )
		{
			strcpy((char*)net_packet->data, "ENTE");
			net_packet->data[4] = clientnum;
			SDLNet_Write32(my->getUID(), &net_packet->data[5]);
			net_packet->address.host = net_server.host;
			net_packet->address.port = net_server.port;
			net_packet->len = 9;
			sendPacketSafe(net_sock, -1, net_packet, 0);
		}
	}

	if ( !PLAYER_INIT )
	{
		PLAYER_INIT = 1;
		my->flags[BURNABLE] = true;

		Entity* nametag = newEntity(-1, 1, map.entities, nullptr);
		nametag->x = my->x;
		nametag->y = my->y;
		nametag->z = my->z - 6;
		nametag->sizex = 1;
		nametag->sizey = 1;
		nametag->flags[NOUPDATE] = true;
		nametag->flags[PASSABLE] = true;
		nametag->flags[SPRITE] = true;
		nametag->flags[BRIGHT] = true;
		nametag->flags[UNCLICKABLE] = true;
		nametag->behavior = &actSpriteNametag;
		nametag->parent = my->getUID();
		nametag->scalex = 0.2;
		nametag->scaley = 0.2;
		nametag->scalez = 0.2;
		if ( multiplayer != CLIENT )
		{
			entity_uids--;
		}
		nametag->setUID(-3);

		// hud weapon
		if ( PLAYER_NUM == clientnum )
		{
			if ( multiplayer == CLIENT )
			{
				my->flags[UPDATENEEDED] = false;
			}

			entity = newEntity(-1, 1, map.entities, nullptr); //HUD entity.
			entity->flags[PASSABLE] = true;
			entity->flags[OVERDRAW] = true;
			entity->flags[NOUPDATE] = true;
			entity->flags[INVISIBLE] = true;
			entity->behavior = &actHudWeapon;
			entity->focalz = -4;
			node = list_AddNodeLast(&my->children);
			node->element = entity;
			node->deconstructor = &emptyDeconstructor;
			node->size = sizeof(Entity*);
			my->bodyparts.push_back(entity);

			// magic hands

			//Left hand.
			entity = newEntity(-1, 1, map.entities, nullptr); //HUD entity.
			entity->flags[PASSABLE] = true;
			entity->flags[OVERDRAW] = true;
			entity->flags[NOUPDATE] = true;
			entity->flags[INVISIBLE] = true;
			entity->behavior = &actLeftHandMagic;
			entity->focalz = -4;
			magicLeftHand = entity;
			//Right hand.
			entity = newEntity(-1, 1, map.entities, nullptr); //HUD entity.
			entity->flags[PASSABLE] = true;
			entity->flags[OVERDRAW] = true;
			entity->flags[NOUPDATE] = true;
			entity->flags[INVISIBLE] = true;
			entity->behavior = &actRightHandMagic;
			entity->focalz = -4;
			magicRightHand = entity;
			my->bodyparts.push_back(entity);

			// hud shield
			entity = newEntity(-1, 1, map.entities, nullptr); //HUD entity.
			entity->flags[PASSABLE] = true;
			entity->flags[OVERDRAW] = true;
			entity->flags[NOUPDATE] = true;
			entity->flags[INVISIBLE] = true;
			entity->behavior = &actHudShield;
			my->bodyparts.push_back(entity);
		}
		else
		{
			node = list_AddNodeLast(&my->children);
			node->element = NULL;
			node->deconstructor = &emptyDeconstructor;
			node->size = 0;
			if ( multiplayer == CLIENT )
			{
				PLAYER_TORCH = 0;
			}
		}

		// torso
		entity = newEntity(spriteTorso, 1, map.entities, nullptr); //Limb entity.
		entity->sizex = 4;
		entity->sizey = 4;
		entity->skill[2] = PLAYER_NUM;
		entity->flags[PASSABLE] = true;
		entity->flags[NOUPDATE] = true;
		entity->flags[GENIUS] = true;
		entity->focalx = limbs[playerRace][1][0];
		entity->focaly = limbs[playerRace][1][1];
		entity->focalz = limbs[playerRace][1][2];
		entity->behavior = &actPlayerLimb;
		entity->parent = my->getUID();
		node = list_AddNodeLast(&my->children);
		node->element = entity;
		node->deconstructor = &emptyDeconstructor;
		node->size = sizeof(Entity*);
		my->bodyparts.push_back(entity);
		entity->setDefaultPlayerModel(PLAYER_NUM, playerRace, LIMB_HUMANOID_TORSO);

		// right leg
		entity = newEntity(spriteLegRight, 1, map.entities, nullptr); //Limb entity.
		entity->sizex = 4;
		entity->sizey = 4;
		entity->skill[2] = PLAYER_NUM;
		entity->flags[PASSABLE] = true;
		entity->flags[NOUPDATE] = true;
		entity->flags[GENIUS] = true;
		entity->focalx = limbs[playerRace][2][0];
		entity->focaly = limbs[playerRace][2][1];
		entity->focalz = limbs[playerRace][2][2];
		entity->behavior = &actPlayerLimb;
		entity->parent = my->getUID();
		node = list_AddNodeLast(&my->children);
		node->element = entity;
		node->deconstructor = &emptyDeconstructor;
		node->size = sizeof(Entity*);
		my->bodyparts.push_back(entity);
		entity->setDefaultPlayerModel(PLAYER_NUM, playerRace, LIMB_HUMANOID_RIGHTLEG);

		// left leg
		entity = newEntity(spriteLegLeft, 1, map.entities, nullptr); //Limb entity.
		entity->sizex = 4;
		entity->sizey = 4;
		entity->skill[2] = PLAYER_NUM;
		entity->flags[PASSABLE] = true;
		entity->flags[NOUPDATE] = true;
		entity->flags[GENIUS] = true;
		entity->focalx = limbs[playerRace][3][0];
		entity->focaly = limbs[playerRace][3][1];
		entity->focalz = limbs[playerRace][3][2];
		entity->behavior = &actPlayerLimb;
		entity->parent = my->getUID();
		node = list_AddNodeLast(&my->children);
		node->element = entity;
		node->deconstructor = &emptyDeconstructor;
		node->size = sizeof(Entity*);
		my->bodyparts.push_back(entity);
		entity->setDefaultPlayerModel(PLAYER_NUM, playerRace, LIMB_HUMANOID_LEFTLEG);

		// right arm
		entity = newEntity(spriteArmRight, 1, map.entities, nullptr); //Limb entity.
		entity->sizex = 4;
		entity->sizey = 4;
		entity->skill[2] = PLAYER_NUM;
		entity->flags[PASSABLE] = true;
		entity->flags[NOUPDATE] = true;
		entity->flags[GENIUS] = true;
		entity->focalx = limbs[playerRace][4][0];
		entity->focaly = limbs[playerRace][4][1];
		entity->focalz = limbs[playerRace][4][2];
		entity->behavior = &actPlayerLimb;
		entity->parent = my->getUID();
		node = list_AddNodeLast(&my->children);
		node->element = entity;
		node->deconstructor = &emptyDeconstructor;
		node->size = sizeof(Entity*);
		my->bodyparts.push_back(entity);
		entity->setDefaultPlayerModel(PLAYER_NUM, playerRace, LIMB_HUMANOID_RIGHTARM);

		// left arm
		entity = newEntity(spriteArmLeft, 1, map.entities, nullptr); //Limb entity.
		entity->sizex = 4;
		entity->sizey = 4;
		entity->skill[2] = PLAYER_NUM;
		entity->flags[PASSABLE] = true;
		entity->flags[NOUPDATE] = true;
		entity->flags[GENIUS] = true;
		entity->focalx = limbs[playerRace][5][0];
		entity->focaly = limbs[playerRace][5][1];
		entity->focalz = limbs[playerRace][5][2];
		entity->behavior = &actPlayerLimb;
		entity->parent = my->getUID();
		node = list_AddNodeLast(&my->children);
		node->element = entity;
		node->deconstructor = &emptyDeconstructor;
		node->size = sizeof(Entity*);
		my->bodyparts.push_back(entity);
		entity->setDefaultPlayerModel(PLAYER_NUM, playerRace, LIMB_HUMANOID_LEFTARM);

		// world weapon
		entity = newEntity(-1, 1, map.entities, nullptr); //Limb entity.
		entity->sizex = 4;
		entity->sizey = 4;
		entity->skill[2] = PLAYER_NUM;
		entity->flags[PASSABLE] = true;
		entity->flags[NOUPDATE] = true;
		entity->flags[GENIUS] = true;
		entity->flags[INVISIBLE] = true;
		entity->focalx = limbs[playerRace][6][0];
		entity->focaly = limbs[playerRace][6][1];
		entity->focalz = limbs[playerRace][6][2];
		entity->behavior = &actPlayerLimb;
		entity->parent = my->getUID();
		node = list_AddNodeLast(&my->children);
		node->element = entity;
		node->deconstructor = &emptyDeconstructor;
		node->size = sizeof(Entity*);
		my->bodyparts.push_back(entity);

		// shield
		entity = newEntity(-1, 1, map.entities, nullptr); //Limb entity.
		entity->sizex = 4;
		entity->sizey = 4;
		entity->skill[2] = PLAYER_NUM;
		entity->flags[PASSABLE] = true;
		entity->flags[NOUPDATE] = true;
		entity->flags[GENIUS] = true;
		entity->flags[INVISIBLE] = true;
		entity->focalx = limbs[playerRace][7][0];
		entity->focaly = limbs[playerRace][7][1];
		entity->focalz = limbs[playerRace][7][2];
		entity->behavior = &actPlayerLimb;
		entity->parent = my->getUID();
		entity->focalx = 2;
		node = list_AddNodeLast(&my->children);
		node->element = entity;
		node->deconstructor = &emptyDeconstructor;
		node->size = sizeof(Entity*);
		my->bodyparts.push_back(entity);

		// cloak
		entity = newEntity(-1, 1, map.entities, nullptr); //Limb entity.
		entity->sizex = 4;
		entity->sizey = 4;
		entity->scalex = 1.01;
		entity->scaley = 1.01;
		entity->scalez = 1.01;
		entity->skill[2] = PLAYER_NUM;
		entity->flags[PASSABLE] = true;
		entity->flags[NOUPDATE] = true;
		entity->flags[GENIUS] = true;
		entity->flags[INVISIBLE] = true;
		entity->focalx = limbs[playerRace][8][0];
		entity->focaly = limbs[playerRace][8][1];
		entity->focalz = limbs[playerRace][8][2];
		entity->behavior = &actPlayerLimb;
		entity->parent = my->getUID();
		node = list_AddNodeLast(&my->children);
		node->element = entity;
		node->deconstructor = &emptyDeconstructor;
		node->size = sizeof(Entity*);
		my->bodyparts.push_back(entity);

		// helmet
		entity = newEntity(-1, 1, map.entities, nullptr); //Limb entity.
		entity->sizex = 4;
		entity->sizey = 4;
		entity->scalex = 1.01;
		entity->scaley = 1.01;
		entity->scalez = 1.01;
		entity->skill[2] = PLAYER_NUM;
		entity->flags[PASSABLE] = true;
		entity->flags[NOUPDATE] = true;
		entity->flags[GENIUS] = true;
		entity->flags[INVISIBLE] = true;
		entity->focalx = limbs[playerRace][9][0];
		entity->focaly = limbs[playerRace][9][1];
		entity->focalz = limbs[playerRace][9][2];
		entity->behavior = &actPlayerLimb;
		entity->parent = my->getUID();
		node = list_AddNodeLast(&my->children);
		node->element = entity;
		node->deconstructor = &emptyDeconstructor;
		node->size = sizeof(Entity*);
		my->bodyparts.push_back(entity);

		// mask
		entity = newEntity(-1, 1, map.entities, nullptr); //Limb entity.
		entity->sizex = 4;
		entity->sizey = 4;
		entity->scalex = .99;
		entity->scaley = .99;
		entity->scalez = .99;
		entity->skill[2] = PLAYER_NUM;
		entity->flags[PASSABLE] = true;
		entity->flags[NOUPDATE] = true;
		entity->flags[GENIUS] = true;
		entity->flags[INVISIBLE] = true;
		entity->focalx = limbs[playerRace][10][0];
		entity->focaly = limbs[playerRace][10][1];
		entity->focalz = limbs[playerRace][10][2];
		entity->behavior = &actPlayerLimb;
		entity->parent = my->getUID();
		node = list_AddNodeLast(&my->children);
		node->element = entity;
		node->deconstructor = &emptyDeconstructor;
		node->size = sizeof(Entity*);
		my->bodyparts.push_back(entity);

		// additional limb 1
		entity = newEntity(-1, 1, map.entities, nullptr); //Limb entity.
		entity->sizex = 1;
		entity->sizey = 1;
		entity->skill[2] = PLAYER_NUM;
		entity->flags[PASSABLE] = true;
		entity->flags[NOUPDATE] = true;
		entity->flags[GENIUS] = true;
		entity->flags[INVISIBLE] = true;
		entity->focalx = limbs[playerRace][11][0];
		entity->focaly = limbs[playerRace][11][1];
		entity->focalz = limbs[playerRace][11][2];
		entity->behavior = &actPlayerLimb;
		entity->parent = my->getUID();
		node = list_AddNodeLast(&my->children);
		node->element = entity;
		node->deconstructor = &emptyDeconstructor;
		node->size = sizeof(Entity*);
		my->bodyparts.push_back(entity);

		// additional limb 2
		entity = newEntity(-1, 1, map.entities, nullptr); //Limb entity.
		entity->sizex = 1;
		entity->sizey = 1;
		entity->skill[2] = PLAYER_NUM;
		entity->flags[PASSABLE] = true;
		entity->flags[NOUPDATE] = true;
		entity->flags[GENIUS] = true;
		entity->flags[INVISIBLE] = true;
		entity->focalx = limbs[playerRace][12][0];
		entity->focaly = limbs[playerRace][12][1];
		entity->focalz = limbs[playerRace][12][2];
		entity->behavior = &actPlayerLimb;
		entity->parent = my->getUID();
		node = list_AddNodeLast(&my->children);
		node->element = entity;
		node->deconstructor = &emptyDeconstructor;
		node->size = sizeof(Entity*);
		my->bodyparts.push_back(entity);
	}
	Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 255);
	int blueSpeechVolume = 100;
	int orangeSpeechVolume = 128;

	if ( !intro )
	{
		PLAYER_ALIVETIME++;
		if ( PLAYER_NUM == clientnum )
		{
			clientplayer = my->getUID();
			if ( !strcmp(map.name, "Boss") && !my->skill[29] )
			{
				bool foundherx = false;
				for ( node = map.creatures->first; node != nullptr; node = node->next ) //Herx is in the creature list, so only search that.
				{
					Entity* entity = (Entity*)node->element;
					if ( entity->sprite == 274 )
					{
						foundherx = true;
						break;
					}
				}
				if ( !foundherx )
				{
					// ding, dong, the witch is dead
					my->skill[29] = PLAYER_ALIVETIME;
				}
				else
				{
					if ( PLAYER_ALIVETIME == 300 )
					{
						playSound(185, 128);
						messagePlayerColor(clientnum, color, language[537]);
						messagePlayerColor(clientnum, color, language[89]);
					}
				}
			}
			else
			{
				if ( PLAYER_ALIVETIME == 300 )
				{
					// five seconds in, herx chimes in (maybe)
					my->playerLevelEntrySpeech = 0;
					if ( currentlevel == 0 && !secretlevel )
					{
						int speech = rand() % 3;
						playSound(126 + speech, 128);
						messagePlayerColor(clientnum, color, language[537]);
						messagePlayerColor(clientnum, color, language[77 + speech]);
					}
					else if ( currentlevel == 1 && !secretlevel )
					{
						int speech = rand() % 3;
						playSound(117 + speech, 128);
						messagePlayerColor(clientnum, color, language[537]);
						messagePlayerColor(clientnum, color, language[70 + speech]);
					}
					else if ( currentlevel == 5 && !secretlevel )
					{
						int speech = rand() % 2;
						playSound(156 + speech, 128);
						messagePlayerColor(clientnum, color, language[537]);
						messagePlayerColor(clientnum, color, language[83 + speech]);
					}
					else if ( currentlevel == 10 && !secretlevel )
					{
						int speech = rand() % 2;
						playSound(158 + speech, 128);
						messagePlayerColor(clientnum, color, language[537]);
						messagePlayerColor(clientnum, color, language[85 + speech]);
					}
					else if ( currentlevel == 15 && !secretlevel )
					{
						int speech = rand() % 2;
						playSound(160 + speech, 128);
						messagePlayerColor(clientnum, color, language[537]);
						messagePlayerColor(clientnum, color, language[87 + speech]);
					}
					else if ( currentlevel == 26 && !secretlevel )
					{
						int speech = 1 + rand() % 3;
						switch ( speech )
						{
							case 1:
								playSound(341, blueSpeechVolume);
								messagePlayerColor(clientnum, uint32ColorBaronyBlue(*mainsurface), language[537]);
								messagePlayerColor(clientnum, uint32ColorBaronyBlue(*mainsurface), language[2615]);
								break;
							case 2:
								playSound(343, orangeSpeechVolume);
								messagePlayerColor(clientnum, uint32ColorOrange(*mainsurface), language[537]);
								messagePlayerColor(clientnum, uint32ColorOrange(*mainsurface), language[2617]);
								break;
							case 3:
								playSound(346, orangeSpeechVolume);
								messagePlayerColor(clientnum, uint32ColorOrange(*mainsurface), language[537]);
								messagePlayerColor(clientnum, uint32ColorOrange(*mainsurface), language[2620]);
								break;
						}
						my->playerLevelEntrySpeech = speech;
					}
					else if ( currentlevel == 28 && !secretlevel )
					{
						int speech = 1 + rand() % 3;
						switch ( speech )
						{
							case 1:
								playSound(349, blueSpeechVolume);
								messagePlayerColor(clientnum, uint32ColorBaronyBlue(*mainsurface), language[537]);
								messagePlayerColor(clientnum, uint32ColorBaronyBlue(*mainsurface), language[2629]);
								break;
							case 2:
								playSound(352, orangeSpeechVolume);
								messagePlayerColor(clientnum, uint32ColorOrange(*mainsurface), language[537]);
								messagePlayerColor(clientnum, uint32ColorOrange(*mainsurface), language[2632]);
								break;
							case 3:
								playSound(354, blueSpeechVolume);
								messagePlayerColor(clientnum, uint32ColorBaronyBlue(*mainsurface), language[537]);
								messagePlayerColor(clientnum, uint32ColorBaronyBlue(*mainsurface), language[2634]);
								break;
						}
						my->playerLevelEntrySpeech = speech;
					}
					else if ( currentlevel == 30 && !secretlevel )
					{
						int speech = 1;
						switch ( speech )
						{
							case 1:
								playSound(356, blueSpeechVolume - 16);
								messagePlayerColor(clientnum, uint32ColorBaronyBlue(*mainsurface), language[537]);
								messagePlayerColor(clientnum, uint32ColorBaronyBlue(*mainsurface), language[2636]);
								break;
						}
						my->playerLevelEntrySpeech = speech;
					}
					else if ( currentlevel == 31 && !secretlevel )
					{
						int speech = 1;
						switch ( speech )
						{
							case 1:
								playSound(358, blueSpeechVolume);
								messagePlayerColor(clientnum, uint32ColorBaronyBlue(*mainsurface), language[537]);
								messagePlayerColor(clientnum, uint32ColorBaronyBlue(*mainsurface), language[2638]);
								break;
						}
						my->playerLevelEntrySpeech = speech;
					}
					else if ( currentlevel == 33 && !secretlevel )
					{
						int speech = 1 + rand() % 2;
						switch ( speech )
						{
							case 1:
								playSound(360, orangeSpeechVolume);
								messagePlayerColor(clientnum, uint32ColorOrange(*mainsurface), language[537]);
								messagePlayerColor(clientnum, uint32ColorOrange(*mainsurface), language[2640]);
								break;
							case 2:
								playSound(362, blueSpeechVolume);
								messagePlayerColor(clientnum, uint32ColorBaronyBlue(*mainsurface), language[537]);
								messagePlayerColor(clientnum, uint32ColorBaronyBlue(*mainsurface), language[2642]);
								break;
						}
						my->playerLevelEntrySpeech = speech;
					}
					else if ( currentlevel == 35 && !secretlevel )
					{
						int speech = 1;
						switch ( speech )
						{
							case 1:
								playSound(364, orangeSpeechVolume);
								messagePlayerColor(clientnum, uint32ColorOrange(*mainsurface), language[537]);
								messagePlayerColor(clientnum, uint32ColorOrange(*mainsurface), language[2644]);
								break;
						}
						my->playerLevelEntrySpeech = speech;
					}
					else if ( minotaurlevel )
					{
						if ( currentlevel < 25 )
						{
							int speech = rand() % 3;
							playSound(123 + speech, 128);
							messagePlayerColor(clientnum, color, language[537]);
							messagePlayerColor(clientnum, color, language[74 + speech]);
						}
						else
						{
							int speech = 1 + rand() % 3;
							switch ( speech )
							{
								case 1:
									playSound(366, blueSpeechVolume);
									messagePlayerColor(clientnum, uint32ColorBaronyBlue(*mainsurface), language[537]);
									messagePlayerColor(clientnum, uint32ColorBaronyBlue(*mainsurface), language[2623]);
									break;
								case 2:
									playSound(368, orangeSpeechVolume);
									messagePlayerColor(clientnum, uint32ColorOrange(*mainsurface), language[537]);
									messagePlayerColor(clientnum, uint32ColorOrange(*mainsurface), language[2625]);
									break;
								case 3:
									playSound(370, blueSpeechVolume);
									messagePlayerColor(clientnum, uint32ColorBaronyBlue(*mainsurface), language[537]);
									messagePlayerColor(clientnum, uint32ColorBaronyBlue(*mainsurface), language[2627]);
									break;
							}
							my->playerLevelEntrySpeech = speech;
						}
					}
				}
				else if ( PLAYER_ALIVETIME == 480 )
				{
					// 8 seconds in, herx chimes in again (maybe)
					if ( currentlevel == 1 && !secretlevel )
					{
						playSound(120 + rand() % 3, 128);
						messagePlayerColor(clientnum, color, language[73]);
					}
					else if ( minotaurlevel && currentlevel < 25 )
					{
						int speech = rand() % 3;
						playSound(129 + speech, 128);
						messagePlayerColor(clientnum, color, language[80 + speech]);
					}
				}
				else if ( my->playerLevelEntrySpeech > 0 )
				{
					my->playerLevelEntrySpeechSecond();
				}
			}

			// shurar the talking mace
			if ( stats[PLAYER_NUM]->weapon )
			{
				if ( stats[PLAYER_NUM]->weapon->type == ARTIFACT_MACE )
				{
					if ( PLAYER_ALIVETIME % 420 == 0 )
					{
						messagePlayerColor(clientnum, color, language[538 + rand() % 32]);
					}
				}
			}

			if ( monsterEmoteGimpTimer > 0 )
			{
				--monsterEmoteGimpTimer;
			}
		}
		if ( multiplayer == SERVER )
		{
			if ( my->getUID() % (TICKS_PER_SECOND * 3) == ticks % (TICKS_PER_SECOND * 3) )
			{
				serverUpdateBodypartIDs(my);

				int i;
				for ( i = 1; i < 11; i++ )
				{
					serverUpdateEntityBodypart(my, i);
				}
			}
		}
		if ( multiplayer != CLIENT )
		{
			if ( PLAYER_ALIVETIME == 50 && currentlevel == 0 )
			{
				int monsterSquad = 0;
				for ( int c = 0; c < MAXPLAYERS; ++c )
				{
					if ( players[c] && players[c]->entity && stats[c]->playerRace > 0 )
					{
						++monsterSquad;
					}
				}
				if ( monsterSquad >= 3 )
				{
					for ( int c = 0; c < MAXPLAYERS; ++c )
					{
						steamAchievementClient(c, "BARONY_ACH_MONSTER_SQUAD");
					}
				}
				if ( client_classes[PLAYER_NUM] == CLASS_ACCURSED )
				{
					my->setEffect(EFF_VAMPIRICAURA, true, -2, true);
					my->playerVampireCurse = 1;
					serverUpdateEntitySkill(my, 51);
					Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
					messagePlayerColor(PLAYER_NUM, color, language[2477]);
					color = SDL_MapRGB(mainsurface->format, 255, 255, 0);
					messagePlayerColor(PLAYER_NUM, color, language[3202]);

					playSoundEntity(my, 167, 128);
					playSoundEntity(my, 403, 128);
					createParticleDropRising(my, 600, 0.7);
					serverSpawnMiscParticles(my, PARTICLE_EFFECT_VAMPIRIC_AURA, 600);
				}
			}
			if ( currentlevel == 0 && stats[PLAYER_NUM]->playerRace == RACE_GOATMAN && stats[PLAYER_NUM]->appearance == 0 )
			{
				if ( PLAYER_ALIVETIME == 1 )
				{
					my->setEffect(EFF_WITHDRAWAL, true, -2, true);
				}
				if ( PLAYER_ALIVETIME == 330 )
				{
					my->setEffect(EFF_ASLEEP, false, 0, true);
					if ( svFlags & SV_FLAG_HUNGER )
					{
						if ( stats[PLAYER_NUM]->HUNGER <= 1000 ) // just in case you ate before scripted sequence
						{
							playSoundPlayer(PLAYER_NUM, 32, 128);
							stats[PLAYER_NUM]->HUNGER = 150;
							serverUpdateHunger(PLAYER_NUM);
						}
						else
						{
							stats[PLAYER_NUM]->HUNGER -= 850;
							serverUpdateHunger(PLAYER_NUM);
						}
					}
				}
				if ( stats[PLAYER_NUM]->EFFECTS[EFF_WITHDRAWAL] )
				{
					if ( PLAYER_ALIVETIME == 500 )
					{
						color = SDL_MapRGB(mainsurface->format, 255, 255, 255);
						messagePlayerColor(PLAYER_NUM, color, language[3221]);
					}
					else if ( PLAYER_ALIVETIME == 700 )
					{
						color = SDL_MapRGB(mainsurface->format, 255, 255, 255);
						messagePlayerColor(PLAYER_NUM, color, language[3222]);
					}
				}
			}
		}
		if ( multiplayer == CLIENT && client_classes[PLAYER_NUM] == CLASS_ACCURSED )
		{
			if ( PLAYER_NUM == clientnum && my->playerVampireCurse == 1 )
			{
				stats[PLAYER_NUM]->EFFECTS_TIMERS[EFF_VAMPIRICAURA] = -2;
			}
			else
			{
				stats[PLAYER_NUM]->EFFECTS_TIMERS[EFF_VAMPIRICAURA] = 0;
			}
		}
	}

	/*if ( my->ticks % 50 == 0 )
	{
		messagePlayer(clientnum, "%d", stats[clientnum]->HUNGER);
	}*/

	if (PLAYER_NUM == clientnum && appraisal_timer > 0)
	{
		Item* tempItem = uidToItem(appraisal_item);
		if ( tempItem )
		{
			if ( tempItem->identified )
			{
				appraisal_timer = 0;
				appraisal_item = 0;
			}
			else if ( tempItem->type == GEM_ROCK )
			{
				//Auto-succeed on rocks.
				tempItem->identified = true;
				messagePlayer(clientnum, language[570], tempItem->description());
				appraisal_item = 0;
				appraisal_timer = 0;

				// update inventory by trying to stack the newly identified item.
				for ( node = stats[PLAYER_NUM]->inventory.first; node != NULL; node = node->next )
				{
					Item* item2 = (Item*)node->element;
					if ( item2 && item2 != tempItem && !itemCompare(tempItem, item2, false) )
					{
						// if items are the same, check to see if they should stack
						if ( item2->shouldItemStack(PLAYER_NUM) )
						{
							item2->count += tempItem->count;
							if ( multiplayer == CLIENT && itemIsEquipped(item2, clientnum) )
							{
								// if incrementing qty and holding item, then send "equip" for server to update their count of your held item.
								strcpy((char*)net_packet->data, "EQUI");
								SDLNet_Write32((Uint32)item2->type, &net_packet->data[4]);
								SDLNet_Write32((Uint32)item2->status, &net_packet->data[8]);
								SDLNet_Write32((Uint32)item2->beatitude, &net_packet->data[12]);
								SDLNet_Write32((Uint32)item2->count, &net_packet->data[16]);
								SDLNet_Write32((Uint32)item2->appearance, &net_packet->data[20]);
								net_packet->data[24] = item2->identified;
								net_packet->data[25] = PLAYER_NUM;
								net_packet->address.host = net_server.host;
								net_packet->address.port = net_server.port;
								net_packet->len = 26;
								sendPacketSafe(net_sock, -1, net_packet, 0);
							}
							if ( tempItem->node )
							{
								list_RemoveNode(tempItem->node);
							}
							else
							{
								free(tempItem);
								tempItem = nullptr;
							}
							break;
						}
					}
				}
			}
			else
			{
				appraisal_timer -= 1; //De-increment appraisal timer.
				if (appraisal_timer <= 0)
				{
					appraisal_timer = 0;

					//Cool. Time to identify the item.
					bool success = false;
					if ( stats[PLAYER_NUM]->PROFICIENCIES[PRO_APPRAISAL] < 100 )
					{
						if ( tempItem->type != GEM_GLASS )
						{
							success = (stats[PLAYER_NUM]->PROFICIENCIES[PRO_APPRAISAL] + my->getPER() * 5 >= items[tempItem->type].value / 10);
						}
						else
						{
							success = (stats[PLAYER_NUM]->PROFICIENCIES[PRO_APPRAISAL] + my->getPER() * 5 >= 100);
						}
					}
					else
					{
						success = true; // always succeed when appraisal is maxed out
					}
					if ( success )
					{
						tempItem->identified = true;
						messagePlayer(clientnum, language[570], tempItem->description());
						if ( tempItem->type == GEM_GLASS )
						{
							steamStatisticUpdate(STEAM_STAT_RHINESTONE_COWBOY, STEAM_STAT_INT, 1);
						}
					}
					else
					{
						if ( itemCategory(tempItem) == GEM )
						{
							messagePlayer(clientnum, language[3240], tempItem->description());
						}
					}

					//Attempt a level up.
					if ( tempItem && items[tempItem->type].value > 0 && stats[PLAYER_NUM] )
					{
						if ( tempItem->identified )
						{
							int appraisalEaseOfDifficulty = 0;
							if ( items[tempItem->type].value < 100 )
							{
								// easy junk items
								appraisalEaseOfDifficulty = 2;
							}
							else if ( items[tempItem->type].value < 200 )
							{
								// medium
								appraisalEaseOfDifficulty = 1;
							}
							else if ( items[tempItem->type].value < 300 )
							{
								// medium
								appraisalEaseOfDifficulty = 0;
							}
							else if ( items[tempItem->type].value < 400 )
							{
								// hardest
								appraisalEaseOfDifficulty = -1;
							}
							else
							{
								// hardest
								appraisalEaseOfDifficulty = -1;
							}
							appraisalEaseOfDifficulty += stats[PLAYER_NUM]->PROFICIENCIES[PRO_APPRAISAL] / 20;
							// difficulty ranges from 1-in-1 to 1-in-6
							appraisalEaseOfDifficulty = std::max(appraisalEaseOfDifficulty, 1);
							//messagePlayer(0, "Appraisal level up chance: 1 in %d", appraisalEaseOfDifficulty);
							if ( rand() % appraisalEaseOfDifficulty == 0 )
							{
								my->increaseSkill(PRO_APPRAISAL);
							}
						}
						else if ( rand() % 7 == 0 )
						{
							my->increaseSkill(PRO_APPRAISAL);
						}
					}

					if ( success )
					{
						// update inventory by trying to stack the newly identified item.
						for ( node = stats[PLAYER_NUM]->inventory.first; node != NULL; node = node->next )
						{
							Item* item2 = (Item*)node->element;
							if ( item2 && item2 != tempItem && !itemCompare(tempItem, item2, false) )
							{
								// if items are the same, check to see if they should stack
								if ( item2->shouldItemStack(PLAYER_NUM) )
								{
									item2->count += tempItem->count;
									if ( multiplayer == CLIENT && itemIsEquipped(item2, clientnum) )
									{
										// if incrementing qty and holding item, then send "equip" for server to update their count of your held item.
										strcpy((char*)net_packet->data, "EQUI");
										SDLNet_Write32((Uint32)item2->type, &net_packet->data[4]);
										SDLNet_Write32((Uint32)item2->status, &net_packet->data[8]);
										SDLNet_Write32((Uint32)item2->beatitude, &net_packet->data[12]);
										SDLNet_Write32((Uint32)item2->count, &net_packet->data[16]);
										SDLNet_Write32((Uint32)item2->appearance, &net_packet->data[20]);
										net_packet->data[24] = item2->identified;
										net_packet->data[25] = PLAYER_NUM;
										net_packet->address.host = net_server.host;
										net_packet->address.port = net_server.port;
										net_packet->len = 26;
										sendPacketSafe(net_sock, -1, net_packet, 0);
									}
									if ( tempItem->node )
									{
										list_RemoveNode(tempItem->node);
									}
									else
									{
										free(tempItem);
										tempItem = nullptr;
									}
									break;
								}
							}
						}
					}

					appraisal_item = 0;
				}
			}
		}
		else
		{
			appraisal_timer = 0;
			appraisal_item = 0;
		}
	}

	// remove broken equipment
	if ( stats[PLAYER_NUM]->helmet != NULL )
		if ( stats[PLAYER_NUM]->helmet->status == BROKEN )
		{
			stats[PLAYER_NUM]->helmet = NULL;
		}
	if ( stats[PLAYER_NUM]->breastplate != NULL )
		if ( stats[PLAYER_NUM]->breastplate->status == BROKEN )
		{
			stats[PLAYER_NUM]->breastplate = NULL;
		}
	if ( stats[PLAYER_NUM]->gloves != NULL )
		if ( stats[PLAYER_NUM]->gloves->status == BROKEN )
		{
			stats[PLAYER_NUM]->gloves = NULL;
		}
	if ( stats[PLAYER_NUM]->shoes != NULL )
		if ( stats[PLAYER_NUM]->shoes->status == BROKEN )
		{
			stats[PLAYER_NUM]->shoes = NULL;
		}
	if ( stats[PLAYER_NUM]->shield != NULL )
		if ( stats[PLAYER_NUM]->shield->status == BROKEN )
		{
			stats[PLAYER_NUM]->shield = NULL;
		}
	if ( stats[PLAYER_NUM]->weapon != NULL )
		if ( stats[PLAYER_NUM]->weapon->status == BROKEN )
		{
			stats[PLAYER_NUM]->weapon = NULL;
		}
	if ( stats[PLAYER_NUM]->cloak != NULL )
		if ( stats[PLAYER_NUM]->cloak->status == BROKEN )
		{
			stats[PLAYER_NUM]->cloak = NULL;
		}
	if ( stats[PLAYER_NUM]->amulet != NULL )
		if ( stats[PLAYER_NUM]->amulet->status == BROKEN )
		{
			stats[PLAYER_NUM]->amulet = NULL;
		}
	if ( stats[PLAYER_NUM]->ring != NULL )
		if ( stats[PLAYER_NUM]->ring->status == BROKEN )
		{
			stats[PLAYER_NUM]->ring = NULL;
		}
	if ( stats[PLAYER_NUM]->mask != NULL )
		if ( stats[PLAYER_NUM]->mask->status == BROKEN )
		{
			stats[PLAYER_NUM]->mask = NULL;
		}

	if ( multiplayer != CLIENT )
	{
		my->effectTimes();
	}

	// invisibility
	if ( !intro )
	{
		if ( PLAYER_NUM == clientnum || multiplayer == SERVER )
		{
			if ( stats[PLAYER_NUM]->ring != NULL )
				if ( stats[PLAYER_NUM]->ring->type == RING_INVISIBILITY )
				{
					wearingring = true;
				}
			if ( stats[PLAYER_NUM]->cloak != NULL )
				if ( stats[PLAYER_NUM]->cloak->type == CLOAK_INVISIBILITY )
				{
					wearingring = true;
				}
			//if ( stats[PLAYER_NUM]->EFFECTS[EFF_INVISIBLE] == true || wearingring == true )
			if ( my->isInvisible() )
			{
				if ( !my->flags[INVISIBLE] )
				{
					my->flags[INVISIBLE] = true;
					serverUpdateEntityFlag(my, INVISIBLE);
				}
				my->flags[BLOCKSIGHT] = false;
				if ( multiplayer != CLIENT )
				{
					for ( i = 0, node = my->children.first; node != NULL; node = node->next, ++i )
					{
						if ( i == 0 )
						{
							continue;
						}
						if ( i >= 6 )
						{
							break;
						}
						entity = (Entity*)node->element;
						if ( !entity->flags[INVISIBLE] )
						{
							entity->flags[INVISIBLE] = true;
							serverUpdateEntityBodypart(my, i);
						}
					}
				}
			}
			else
			{
				if ( my->flags[INVISIBLE] )
				{
					my->flags[INVISIBLE] = false;
					serverUpdateEntityFlag(my, INVISIBLE);
				}
				my->flags[BLOCKSIGHT] = true;
				if ( multiplayer != CLIENT )
				{
					for (i = 0, node = my->children.first; node != NULL; node = node->next, i++)
					{
						if ( i == 0 )
						{
							continue;
						}
						if ( i >= 6 )
						{
							break;
						}
						entity = (Entity*)node->element;
						if ( entity->flags[INVISIBLE] )
						{
							entity->flags[INVISIBLE] = false;
							serverUpdateEntityBodypart(my, i);
						}
					}
				}
			}
		}
	}

	if ( PLAYER_NUM == clientnum || multiplayer == SERVER )
	{
		bool prevlevitating = false;
		if ( multiplayer != CLIENT )
		{
			if ( (my->z >= -2.05 && my->z <= -1.95) || (my->z >= -1.55 && my->z <= -1.45) )
			{
				prevlevitating = true;
			}
		}

		// sleeping
		if ( stats[PLAYER_NUM]->EFFECTS[EFF_ASLEEP] )
		{
			switch ( stats[PLAYER_NUM]->type )
			{
				case GOBLIN:
				case GOATMAN:
				case INSECTOID:
					my->z = 2.5;
					break;
				case SKELETON:
				case AUTOMATON:
					my->z = 2.f;
					break;
				case HUMAN:
				case VAMPIRE:
				case INCUBUS:
				case SUCCUBUS:
					my->z = 1.5;
					break;
				default:
					my->z = 1.5;
					break;
			}
			my->pitch = PI / 4;
		}
		else if ( !noclip )
		{
			my->z = -1;
			if ( intro )
			{
				my->pitch = 0;
			}
		}

		// levitation
		levitating = isLevitating(stats[PLAYER_NUM]);

		if ( levitating )
		{
			my->z -= 1; // floating
		}

		if ( !levitating && prevlevitating )
		{
			int x, y, u, v;
			x = std::min(std::max<unsigned int>(1, my->x / 16), map.width - 2);
			y = std::min(std::max<unsigned int>(1, my->y / 16), map.height - 2);
			for ( u = x - 1; u <= x + 1; u++ )
			{
				for ( v = y - 1; v <= y + 1; v++ )
				{
					if ( entityInsideTile(my, u, v, 0) )   // no floor
					{
						//messagePlayer(PLAYER_NUM, language[572]);
						my->setObituary(language[3010]); // fell to their death.
						stats[PLAYER_NUM]->HP = 0; // kill me instantly
						break;
					}
				}
				if ( stats[PLAYER_NUM]->HP == 0 )   //TODO: <= 0?
				{
					break;
				}
			}
		}
	}

	// swimming
	bool waterwalkingboots = false;
	if ( stats[PLAYER_NUM]->shoes != NULL )
		if ( stats[PLAYER_NUM]->shoes->type == IRON_BOOTS_WATERWALKING )
		{
			waterwalkingboots = true;
		}
	bool swimming = false;
	if ( PLAYER_NUM == clientnum || multiplayer == SERVER )
	{
		if ( !levitating && !waterwalkingboots && !noclip && !skillCapstoneUnlocked(PLAYER_NUM, PRO_SWIMMING) )
		{
			int x = std::min(std::max<unsigned int>(0, floor(my->x / 16)), map.width - 1);
			int y = std::min(std::max<unsigned int>(0, floor(my->y / 16)), map.height - 1);
			if ( swimmingtiles[map.tiles[y * MAPLAYERS + x * MAPLAYERS * map.height]]
				|| lavatiles[map.tiles[y * MAPLAYERS + x * MAPLAYERS * map.height]] )
			{
				// can swim in lavatiles or swimmingtiles only.
				if ( rand() % 400 == 0 && multiplayer != CLIENT )
				{
					my->increaseSkill(PRO_SWIMMING);
				}
				swimming = true;
				my->z = 7;
				if ( !PLAYER_INWATER && PLAYER_NUM == clientnum )
				{
					PLAYER_INWATER = 1;
					if ( lavatiles[map.tiles[y * MAPLAYERS + x * MAPLAYERS * map.height]] )
					{
						messagePlayer(PLAYER_NUM, language[573]);
					}
					else if ( swimmingtiles[map.tiles[y * MAPLAYERS + x * MAPLAYERS * map.height]] && stats[PLAYER_NUM]->type == VAMPIRE )
					{
						messagePlayerColor(PLAYER_NUM, SDL_MapRGB(mainsurface->format, 255, 0, 0), language[3183]);
						playSoundPlayer(PLAYER_NUM, 28, 128);
						playSoundPlayer(PLAYER_NUM, 249, 128);
						camera_shakex += .1;
						camera_shakey += 10;
					}
					else if ( swimmingtiles[map.tiles[y * MAPLAYERS + x * MAPLAYERS * map.height]] )
					{
						playSound(136, 128);
					}
				}
				if ( multiplayer != CLIENT )
				{
					// Check if the Player is in Water or Lava
					if ( swimmingtiles[map.tiles[y * MAPLAYERS + x * MAPLAYERS * map.height]] )
					{
						if ( my->flags[BURNING] )
						{
							my->flags[BURNING] = false;
							messagePlayer(PLAYER_NUM, language[574]); // "The water extinguishes the flames!"
							serverUpdateEntityFlag(my, BURNING);
						}
						if ( stats[PLAYER_NUM]->EFFECTS[EFF_POLYMORPH] )
						{
							my->setEffect(EFF_POLYMORPH, false, 0, true);
							my->effectPolymorph = 0;
							serverUpdateEntitySkill(my, 50);

							messagePlayer(PLAYER_NUM, language[3192]);
							messagePlayer(PLAYER_NUM, language[3185]);

							playSoundEntity(my, 400, 92);
							createParticleDropRising(my, 593, 1.f);
							serverSpawnMiscParticles(my, PARTICLE_EFFECT_RISING_DROP, 593);
						}
						if ( stats[PLAYER_NUM]->type == VAMPIRE )
						{
							if ( ticks % 10 == 0 ) // Water deals damage every 10 ticks
							{
								my->modHP(-2 - rand() % 2);
								if ( ticks % 20 == 0 )
								{
									playSoundPlayer(PLAYER_NUM, 28, 92);
								}
								my->setObituary(language[3254]); // "goes for a swim in some water."
								steamAchievementClient(PLAYER_NUM, "BARONY_ACH_BLOOD_BOIL");
							}
						}
					}
					else if ( ticks % 10 == 0 ) // Lava deals damage every 10 ticks
					{
						my->modHP(-2 - rand() % 2);
						my->setObituary(language[1506]); // "goes for a swim in some lava."
						if ( !my->flags[BURNING] )
						{
							// Attempt to set the Entity on fire
							my->SetEntityOnFire();
						}
					}
				}
			}
		}
	}
	if (!swimming)
		if (PLAYER_INWATER)
		{
			PLAYER_INWATER = 0;
		}

	if (PLAYER_NUM == clientnum)
	{
		players[PLAYER_NUM]->entity = my;

		// camera bobbing
		if (bobbing)
		{
			if ( swimming )
			{
				if ( PLAYER_BOBMODE )
				{
					PLAYER_BOBMOVE += .03;
				}
				else
				{
					PLAYER_BOBMOVE -= .03;
				}
			}
			if ( ((*inputPressed(impulses[IN_FORWARD]) || *inputPressed(impulses[IN_BACK])) || (*inputPressed(impulses[IN_RIGHT]) - *inputPressed(impulses[IN_LEFT])) || (game_controller && (game_controller->getLeftXPercent() || game_controller->getLeftYPercent()))) && !command && !swimming)
			{
				if ( !(stats[clientnum]->defending || stats[clientnum]->sneaking == 0) )
				{
					if (PLAYER_BOBMODE)
					{
						PLAYER_BOBMOVE += .05;
					}
					else
					{
						PLAYER_BOBMOVE -= .05;
					}
				}
				else
				{
					if (PLAYER_BOBMODE)
					{
						PLAYER_BOBMOVE += .025;
					}
					else
					{
						PLAYER_BOBMOVE -= .025;
					}
				}
			}
			else if ( !swimming )
			{
				PLAYER_BOBMOVE = 0;
				PLAYER_BOB = 0;
				PLAYER_BOBMODE = 0;
			}
			if ( !swimming && !(stats[clientnum]->defending || stats[clientnum]->sneaking == 0) )
			{
				if ( PLAYER_BOBMOVE > .2 )
				{
					PLAYER_BOBMOVE = .2;
					PLAYER_BOBMODE = 0;
				}
				else if ( PLAYER_BOBMOVE < -.2 )
				{
					PLAYER_BOBMOVE = -.2;
					PLAYER_BOBMODE = 1;
				}
			}
			else if ( swimming )
			{
				if ( PLAYER_BOBMOVE > .3 )
				{
					PLAYER_BOBMOVE = .3;
					PLAYER_BOBMODE = 0;
				}
				else if ( PLAYER_BOBMOVE < -.3 )
				{
					PLAYER_BOBMOVE = -.3;
					PLAYER_BOBMODE = 1;
				}
			}
			else
			{
				if ( PLAYER_BOBMOVE > .1 )
				{
					PLAYER_BOBMOVE = .1;
					PLAYER_BOBMODE = 0;
				}
				else if ( PLAYER_BOBMOVE < -.1 )
				{
					PLAYER_BOBMOVE = -.1;
					PLAYER_BOBMODE = 1;
				}
			}
			PLAYER_BOB += PLAYER_BOBMOVE;
		}
		else
		{
			PLAYER_BOB = 0;
		}

		// object interaction
		if ( intro == false )
		{
			clickDescription(PLAYER_NUM, NULL); // inspecting objects

			if ( FollowerMenu.optionSelected == ALLY_CMD_ATTACK_SELECT )
			{
				Entity* underMouse = nullptr;
				if ( FollowerMenu.optionSelected == ALLY_CMD_ATTACK_SELECT && ticks % 10 == 0 )
				{
					if ( !shootmode )
					{
						Uint32 uidnum = GO_GetPixelU32(omousex, yres - omousey);
						if ( uidnum > 0 )
						{
							underMouse = uidToEntity(uidnum);
						}
					}
					else
					{
						Uint32 uidnum = GO_GetPixelU32(xres / 2, yres / 2);
						if ( uidnum > 0 )
						{
							underMouse = uidToEntity(uidnum);
						}
					}

					if ( underMouse && FollowerMenu.followerToCommand )
					{
						Entity* parent = uidToEntity(underMouse->skill[2]);
						if ( underMouse->behavior == &actMonster || (parent && parent->behavior == &actMonster) )
						{
							// see if we selected a limb
							if ( parent )
							{
								underMouse = parent;
							}
						}
						FollowerMenu.allowedInteractEntity(*underMouse);
					}
					else
					{
						strcpy(FollowerMenu.interactText, "");
					}
				}
			}

			if ( FollowerMenu.followerToCommand == nullptr && FollowerMenu.selectMoveTo == false )
			{
				selectedEntity = entityClicked(); // using objects
			}
			else
			{
				selectedEntity = NULL;

				if ( !command && (*inputPressed(impulses[IN_USE]) || *inputPressed(joyimpulses[INJOY_GAME_USE])) )
				{
					if ( !FollowerMenu.menuToggleClick && FollowerMenu.selectMoveTo )
					{
						if ( FollowerMenu.optionSelected == ALLY_CMD_MOVETO_SELECT )
						{
							// we're selecting a point for the ally to move to.
							*inputPressed(impulses[IN_USE]) = 0;
							*inputPressed(joyimpulses[INJOY_GAME_USE]) = 0;

							int minimapTotalScale = minimapScaleQuickToggle + minimapScale;
							if ( mouseInBounds(xres - map.width * minimapTotalScale, xres, yres - map.height * minimapTotalScale, yres) ) // mouse within minimap pixels (each map tile is 4 pixels)
							{
								MinimapPing newPing(ticks, -1, (omousex - (xres - map.width * minimapTotalScale)) / minimapTotalScale, (omousey - (yres - map.height * minimapTotalScale)) / minimapTotalScale);
								minimapPingAdd(newPing);
								createParticleFollowerCommand(newPing.x, newPing.y, 0, 174);
								FollowerMenu.optionSelected = ALLY_CMD_MOVETO_CONFIRM;
								FollowerMenu.selectMoveTo = false;
								FollowerMenu.moveToX = static_cast<int>(newPing.x);
								FollowerMenu.moveToY = static_cast<int>(newPing.y);
							}
							else if ( players[PLAYER_NUM] && players[PLAYER_NUM]->entity )
							{
								real_t startx = players[PLAYER_NUM]->entity->x;
								real_t starty = players[PLAYER_NUM]->entity->y;
								real_t startz = -4;
								real_t pitch = players[PLAYER_NUM]->entity->pitch;
								if ( pitch < 0 )
								{
									pitch = 0;
								}
								// draw line from the players height and direction until we hit the ground.
								real_t previousx = startx;
								real_t previousy = starty;
								int index = 0;
								for ( ; startz < 0.f; startz += abs(0.05 * tan(pitch)) )
								{
									startx += 0.1 * cos(players[PLAYER_NUM]->entity->yaw);
									starty += 0.1 * sin(players[PLAYER_NUM]->entity->yaw);
									index = (static_cast<int>(starty + 16 * sin(players[PLAYER_NUM]->entity->yaw)) >> 4) * MAPLAYERS + (static_cast<int>(startx + 16 * cos(players[PLAYER_NUM]->entity->yaw)) >> 4) * MAPLAYERS * map.height;
									if ( map.tiles[index] && !map.tiles[OBSTACLELAYER + index] )
									{
										// store the last known good coordinate
										previousx = startx;
										previousy = starty;
									}
									if ( map.tiles[OBSTACLELAYER + index] )
									{
										break;
									}
								}

								createParticleFollowerCommand(previousx, previousy, 0, 174);
								FollowerMenu.optionSelected = ALLY_CMD_MOVETO_CONFIRM;
								FollowerMenu.selectMoveTo = false;
								FollowerMenu.moveToX = static_cast<int>(previousx) / 16;
								FollowerMenu.moveToY = static_cast<int>(previousy) / 16;
								//messagePlayer(PLAYER_NUM, "%d, %d, pitch: %f", followerMoveToX, followerMoveToY, players[PLAYER_NUM]->entity->pitch);
							}
						}
						else if ( FollowerMenu.optionSelected == ALLY_CMD_ATTACK_SELECT )
						{
							// we're selecting a target for the ally.
							Entity* target = entityClicked();
							*inputPressed(impulses[IN_USE]) = 0;
							*inputPressed(joyimpulses[INJOY_GAME_USE]) = 0;
							if ( target )
							{
								Entity* parent = uidToEntity(target->skill[2]);
								if ( target->behavior == &actMonster || (parent && parent->behavior == &actMonster) )
								{
									// see if we selected a limb
									if ( parent )
									{
										target = parent;
									}
								}
								else if ( target->sprite == 184 ) // switch base.
								{
									parent = uidToEntity(target->parent);
									if ( parent )
									{
										target = parent;
									}
								}
								if ( FollowerMenu.allowedInteractEntity(*target) )
								{
									createParticleFollowerCommand(target->x, target->y, 0, 174);
									FollowerMenu.optionSelected = ALLY_CMD_ATTACK_CONFIRM;
									FollowerMenu.followerToCommand->monsterAllyInteractTarget = target->getUID();
								}
								else
								{
									messagePlayer(clientnum, language[3094]);
									FollowerMenu.optionSelected = ALLY_CMD_CANCEL;
									FollowerMenu.optionPrevious = ALLY_CMD_ATTACK_CONFIRM;
									FollowerMenu.followerToCommand->monsterAllyInteractTarget = 0;
								}
							}
							else
							{
								FollowerMenu.optionSelected = ALLY_CMD_CANCEL;
								FollowerMenu.optionPrevious = ALLY_CMD_ATTACK_CONFIRM;
								FollowerMenu.followerToCommand->monsterAllyInteractTarget = 0;
							}
							FollowerMenu.selectMoveTo = false;
							strcpy(FollowerMenu.interactText, "");
						}
					}
				}
			}

			if ( !command && !FollowerMenu.followerToCommand && FollowerMenu.recentEntity )
			{
				if ( *inputPressed(impulses[IN_FOLLOWERMENU]) || *inputPressed(joyimpulses[INJOY_GAME_FOLLOWERMENU]) )
				{
					if ( players[PLAYER_NUM] && players[PLAYER_NUM]->entity
						&& FollowerMenu.recentEntity->monsterTarget == players[PLAYER_NUM]->entity->getUID() )
					{
						// your ally is angry at you!
					}
					else
					{
						selectedEntity = FollowerMenu.recentEntity;
						FollowerMenu.holdWheel = true;
					}
				}
				else if ( *inputPressed(impulses[IN_FOLLOWERMENU_LASTCMD]) || *inputPressed(joyimpulses[INJOY_GAME_FOLLOWERMENU_LASTCMD]) )
				{
					if ( players[PLAYER_NUM] && players[PLAYER_NUM]->entity
						&& FollowerMenu.recentEntity->monsterTarget == players[PLAYER_NUM]->entity->getUID() )
					{
						// your ally is angry at you!
						*inputPressed(impulses[IN_FOLLOWERMENU_LASTCMD]) = 0;
						*inputPressed(joyimpulses[INJOY_GAME_FOLLOWERMENU_LASTCMD]) = 0;
					}
					else if ( FollowerMenu.optionPrevious != -1 )
					{
						FollowerMenu.followerToCommand = FollowerMenu.recentEntity;
					}
					else
					{
						*inputPressed(impulses[IN_FOLLOWERMENU_LASTCMD]) = 0;
						*inputPressed(joyimpulses[INJOY_GAME_FOLLOWERMENU_LASTCMD]) = 0;
					}
				}
			}
			if ( selectedEntity != NULL )
			{
				FollowerMenu.followerToCommand = nullptr;
				Entity* parent = uidToEntity(selectedEntity->skill[2]);
				if ( selectedEntity->behavior == &actMonster || (parent && parent->behavior == &actMonster) )
				{
					// see if we selected a follower to process right click menu.
					if ( parent && parent->monsterAllyIndex == PLAYER_NUM )
					{
						FollowerMenu.followerToCommand = parent;
						//messagePlayer(0, "limb");
					}
					else if ( selectedEntity->monsterAllyIndex == PLAYER_NUM )
					{
						FollowerMenu.followerToCommand = selectedEntity;
						//messagePlayer(0, "head");
					}

					if ( FollowerMenu.followerToCommand )
					{
						if ( players[PLAYER_NUM] && players[PLAYER_NUM]->entity
							&& FollowerMenu.followerToCommand->monsterTarget == players[PLAYER_NUM]->entity->getUID() )
						{
							// your ally is angry at you!
							FollowerMenu.followerToCommand = nullptr;
							FollowerMenu.optionPrevious = -1;
						}
						else
						{
							FollowerMenu.recentEntity = FollowerMenu.followerToCommand;
							FollowerMenu.initFollowerMenuGUICursor(true);
							FollowerMenu.updateScrollPartySheet();
							selectedEntity = NULL;
						}
					}
				}
				if ( selectedEntity )
				{
					*inputPressed(impulses[IN_USE]) = 0;
					*inputPressed(joyimpulses[INJOY_GAME_USE]) = 0;
					if ( entityDist(my, selectedEntity) <= TOUCHRANGE )
					{
						inrange[PLAYER_NUM] = true;
					}
					else
					{
						inrange[PLAYER_NUM] = false;
					}
					if ( multiplayer == CLIENT )
					{
						if ( inrange[PLAYER_NUM] )
						{
							strcpy((char*)net_packet->data, "CKIR");
						}
						else
						{
							strcpy((char*)net_packet->data, "CKOR");
						}
						net_packet->data[4] = PLAYER_NUM;
						if (selectedEntity->behavior == &actPlayerLimb)
						{
							SDLNet_Write32((Uint32)players[selectedEntity->skill[2]]->entity->getUID(), &net_packet->data[5]);
						}
						else
						{
							Entity* tempEntity = uidToEntity(selectedEntity->skill[2]);
							if (tempEntity)
							{
								if (tempEntity->behavior == &actMonster)
								{
									SDLNet_Write32((Uint32)tempEntity->getUID(), &net_packet->data[5]);
								}
								else
								{
									SDLNet_Write32((Uint32)selectedEntity->getUID(), &net_packet->data[5]);
								}
							}
							else
							{
								SDLNet_Write32((Uint32)selectedEntity->getUID(), &net_packet->data[5]);
							}
						}
						net_packet->address.host = net_server.host;
						net_packet->address.port = net_server.port;
						net_packet->len = 9;
						sendPacketSafe(net_sock, -1, net_packet, 0);
					}
				}
			}
		}
	}

	if (multiplayer != CLIENT)
	{
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if ((i == 0 && selectedEntity == my) || (client_selected[i] == my) || i == PLAYER_CLICKED - 1)
			{
				PLAYER_CLICKED = 0;
				if (inrange[i] && i != PLAYER_NUM)
				{
					messagePlayer(i, language[575], stats[PLAYER_NUM]->name, stats[PLAYER_NUM]->HP, stats[PLAYER_NUM]->MAXHP, stats[PLAYER_NUM]->MP, stats[PLAYER_NUM]->MAXMP);
					messagePlayer(PLAYER_NUM, language[576], stats[i]->name);
					if (PLAYER_NUM == clientnum && players[i] && players[i]->entity)
					{
						double tangent = atan2(my->y - players[i]->entity->y, my->x - players[i]->entity->x);
						PLAYER_VELX += cos(tangent);
						PLAYER_VELY += sin(tangent);
					}
				}
			}
		}
	}

	// torch light
	if ( !intro )
	{
		if ( multiplayer == SERVER || PLAYER_NUM == clientnum )
		{
			if ( stats[PLAYER_NUM]->shield != NULL )
			{
				if ( PLAYER_NUM == clientnum )
				{
					if ( stats[PLAYER_NUM]->shield->type == TOOL_TORCH )
					{
						PLAYER_TORCH = 7 + my->getPER() / 3 + (stats[PLAYER_NUM]->defending) * 1;
					}
					else if ( stats[PLAYER_NUM]->shield->type == TOOL_LANTERN )
					{
						PLAYER_TORCH = 10 + my->getPER() / 3 + (stats[PLAYER_NUM]->defending) * 1;
					}
					else if ( stats[PLAYER_NUM]->shield->type == TOOL_CRYSTALSHARD )
					{
						PLAYER_TORCH = 5 + my->getPER() / 3 + (stats[PLAYER_NUM]->defending) * 2;
					}
					else if ( !PLAYER_DEBUGCAM )
					{
						PLAYER_TORCH = 3 + my->getPER() / 3;
					}
					else
					{
						PLAYER_TORCH = 0;
					}
				}
				else
				{
					if ( stats[PLAYER_NUM]->shield->type == TOOL_TORCH )
					{
						PLAYER_TORCH = 7;
					}
					else if ( stats[PLAYER_NUM]->shield->type == TOOL_LANTERN )
					{
						PLAYER_TORCH = 10;
					}
					else if ( stats[PLAYER_NUM]->shield->type == TOOL_CRYSTALSHARD )
					{
						PLAYER_TORCH = 5;
					}
					else
					{
						PLAYER_TORCH = 0;
					}
				}
			}
			else
			{
				if ( PLAYER_NUM == clientnum && !PLAYER_DEBUGCAM )
				{
					PLAYER_TORCH = 3 + (my->getPER() / 3);
					// more visible world if defending/sneaking with no shield
					PLAYER_TORCH += ((stats[PLAYER_NUM]->sneaking == 1) * (2 + (stats[PLAYER_NUM]->PROFICIENCIES[PRO_STEALTH] / 40)));
				}
				else
				{
					PLAYER_TORCH = 0;
				}
			}
		}
	}
	else
	{
		PLAYER_TORCH = 0;
	}

	my->removeLightField();

	if ( PLAYER_TORCH && my->light == NULL )
	{
		my->light = lightSphereShadow(my->x / 16, my->y / 16, PLAYER_TORCH, 50 + 15 * PLAYER_TORCH);
	}

	// server controls players primarily
	if ( PLAYER_NUM == clientnum || multiplayer == SERVER )
	{
		// set head model
		if ( playerRace != HUMAN )
		{
			if ( playerRace == SKELETON )
			{
				my->sprite = 686;
			}
			else if ( playerRace == GOBLIN )
			{
				if ( stats[PLAYER_NUM]->sex == FEMALE )
				{
					my->sprite = 752;
				}
				else
				{
					my->sprite = 694;
				}
			}
			else if ( playerRace == INCUBUS )
			{
				my->sprite = 702;
			}
			else if ( playerRace == SUCCUBUS )
			{
				my->sprite = 710;
			}
			else if ( playerRace == VAMPIRE )
			{
				if ( stats[PLAYER_NUM]->sex == FEMALE )
				{
					my->sprite = 756;
				}
				else
				{
					my->sprite = 718;
				}
			}
			else if ( playerRace == INSECTOID )
			{
				if ( stats[PLAYER_NUM]->sex == FEMALE )
				{
					my->sprite = 760;
				}
				else
				{
					my->sprite = 726;
				}
			}
			else if ( playerRace == GOATMAN )
			{
				if ( stats[PLAYER_NUM]->sex == FEMALE )
				{
					my->sprite = 768;
				}
				else
				{
					my->sprite = 734;
				}
			}
			else if ( playerRace == AUTOMATON )
			{
				if ( stats[PLAYER_NUM]->sex == FEMALE )
				{
					my->sprite = 770;
				}
				else
				{
					my->sprite = 742;
				}
			}
		}
		else if ( playerAppearance < 5 )
		{
			my->sprite = 113 + 12 * stats[PLAYER_NUM]->sex + playerAppearance;
		}
		else if ( playerAppearance == 5 )
		{
			my->sprite = 332 + stats[PLAYER_NUM]->sex;
		}
		else if ( playerAppearance >= 6 && playerAppearance < 12 )
		{
			my->sprite = 341 + stats[PLAYER_NUM]->sex * 13 + playerAppearance - 6;
		}
		else if ( playerAppearance >= 12 )
		{
			my->sprite = 367 + stats[PLAYER_NUM]->sex * 13 + playerAppearance - 12;
		}
		else
		{
			my->sprite = 113; // default
		}
	}
	if ( multiplayer != CLIENT )
	{
		// remove client entities that should no longer exist
		if ( !intro )
		{
			my->handleEffects(stats[PLAYER_NUM]); // hunger, regaining hp/mp, poison, etc.
			if ( ticks % (TICKS_PER_SECOND * 3) == 0 )
			{
				// send update to all clients for global stats[NUMPLAYERS] struct
				serverUpdatePlayerStats();
			}
			if ( client_disconnected[PLAYER_NUM] || stats[PLAYER_NUM]->HP <= 0 )
			{
				// remove body parts
				node_t* nextnode;
				for ( node = my->children.first, i = 0; node != NULL; node = nextnode, i++ )
				{
					nextnode = node->next;
					if ( i == 0 )
					{
						continue;
					}
					if ( node->element )
					{
						Entity* tempEntity = (Entity*)node->element;
						if ( tempEntity )
						{
							list_RemoveNode(tempEntity->mynode);
						}
						if ( i > 12 )
						{
							break;
						}
					}
				}
				if ( stats[PLAYER_NUM]->HP <= 0 )
				{
					// die //TODO: Refactor.
					playSoundEntity(my, 28, 128);
					for ( i = 0; i < 5; i++ )
					{
						Entity* gib = spawnGib(my);
						serverSpawnGibForClient(gib);
					}
					if (spawn_blood)
					{
						if ( !checkObstacle(my->x, my->y, my, NULL) )
						{
							int x, y;
							x = std::min(std::max<unsigned int>(0, my->x / 16), map.width - 1);
							y = std::min(std::max<unsigned int>(0, my->y / 16), map.height - 1);
							if ( map.tiles[y * MAPLAYERS + x * MAPLAYERS * map.height] )
							{
								entity = newEntity(160, 1, map.entities, nullptr); //Limb entity.
								entity->x = my->x;
								entity->y = my->y;
								entity->z = 8.0 + (rand() % 20) / 100.0;
								entity->parent = my->getUID();
								entity->sizex = 2;
								entity->sizey = 2;
								entity->yaw = (rand() % 360) * PI / 180.0;
								entity->flags[UPDATENEEDED] = true;
								entity->flags[PASSABLE] = true;
							}
						}
					}
					node_t* spellnode;
					spellnode = stats[PLAYER_NUM]->magic_effects.first;
					while (spellnode)
					{
						node_t* oldnode = spellnode;
						spellnode = spellnode->next;
						spell_t* spell = (spell_t*)oldnode->element;
						spell->magic_effects_node = NULL;
						list_RemoveNode(oldnode);
					}
					int c;
					for ( c = 0; c < MAXPLAYERS; c++ )
					{
						if ( client_disconnected[c] )
						{
							continue;
						}
						char whatever[256];
						snprintf(whatever, 255, "%s %s", stats[PLAYER_NUM]->name, stats[PLAYER_NUM]->obituary); //Potential snprintf of 256 bytes into 255 byte destination
						messagePlayer(c, whatever);
					}
					Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
					messagePlayerColor(PLAYER_NUM, color, language[577]);

					for ( node_t* node = stats[PLAYER_NUM]->FOLLOWERS.first; node != nullptr; node = nextnode )
					{
						nextnode = node->next;
						Uint32* c = (Uint32*)node->element;
						Entity* myFollower = uidToEntity(*c);
						if ( myFollower )
						{
							if ( myFollower->monsterAllySummonRank != 0 )
							{
								myFollower->setMP(0);
								myFollower->setHP(0); // rip
							}
							else if ( myFollower->flags[USERFLAG2] )
							{
								// our leader died, let's undo the color change since we're now rabid.
								myFollower->flags[USERFLAG2] = false;
								serverUpdateEntityFlag(myFollower, USERFLAG2);

								int bodypart = 0;
								for ( node_t* node = myFollower->children.first; node != nullptr; node = node->next )
								{
									if ( bodypart >= LIMB_HUMANOID_TORSO )
									{
										Entity* tmp = (Entity*)node->element;
										if ( tmp )
										{
											tmp->flags[USERFLAG2] = false;
										}
									}
									++bodypart;
								}

								Stat* followerStats = myFollower->getStats();
								if ( followerStats )
								{
									followerStats->leader_uid = 0;
								}
								list_RemoveNode(node);
								if ( PLAYER_NUM != clientnum )
								{
									serverRemoveClientFollower(PLAYER_NUM, myFollower->getUID());
								}
							}
						}

					}

					/* //TODO: Eventually.
					{
						strcpy((char *)net_packet->data,"UDIE");
						net_packet->address.host = net_clients[player-1].host;
						net_packet->address.port = net_clients[player-1].port;
						net_packet->len = 4;
						sendPacketSafe(net_sock, -1, net_packet, player-1);
					}
					*/
					if ( clientnum == PLAYER_NUM )
					{
						// deathcam
						entity = newEntity(-1, 1, map.entities, nullptr); //Deathcam entity.
						entity->x = my->x;
						entity->y = my->y;
						entity->z = -2;
						entity->flags[NOUPDATE] = true;
						entity->flags[PASSABLE] = true;
						entity->flags[INVISIBLE] = true;
						entity->behavior = &actDeathCam;
						entity->yaw = my->yaw;
						entity->pitch = PI / 8;
						node_t* nextnode;

						if ( multiplayer == SINGLE )
						{
							deleteSaveGame(multiplayer); // stops save scumming c:
						}
						else
						{
							deleteMultiplayerSaveGames(); //Will only delete save games if was last player alive.
						}

						closeBookGUI();

#ifdef SOUND
						levelmusicplaying = true;
						combatmusicplaying = false;
						fadein_increment = default_fadein_increment * 4;
						fadeout_increment = default_fadeout_increment * 4;
						playmusic(sounds[209], false, true, false);
#endif
						combat = false;
						if ( multiplayer == SINGLE || !(svFlags & SV_FLAG_KEEPINVENTORY) )
						{
							for (node = stats[PLAYER_NUM]->inventory.first; node != nullptr; node = nextnode)
							{
								nextnode = node->next;
								Item* item = (Item*)node->element;
								if (itemCategory(item) == SPELL_CAT)
								{
									continue;    // don't drop spells on death, stupid!
								}
								if ( item->type >= ARTIFACT_SWORD && item->type <= ARTIFACT_GLOVES )
								{
									if ( itemIsEquipped(item, clientnum) )
									{
										steamAchievement("BARONY_ACH_CHOSEN_ONE");
									}
								}
								int c = item->count;
								for (c = item->count; c > 0; c--)
								{
									entity = newEntity(-1, 1, map.entities, nullptr); //Item entity.
									entity->flags[INVISIBLE] = true;
									entity->flags[UPDATENEEDED] = true;
									entity->x = my->x;
									entity->y = my->y;
									entity->sizex = 4;
									entity->sizey = 4;
									entity->yaw = (rand() % 360) * (PI / 180.f);
									entity->vel_x = (rand() % 20 - 10) / 10.0;
									entity->vel_y = (rand() % 20 - 10) / 10.0;
									entity->vel_z = -.5;
									entity->flags[PASSABLE] = true;
									entity->flags[USERFLAG1] = true;
									entity->behavior = &actItem;
									entity->skill[10] = item->type;
									entity->skill[11] = item->status;
									entity->skill[12] = item->beatitude;
									entity->skill[13] = 1;
									entity->skill[14] = item->appearance;
									entity->skill[15] = item->identified;
								}
							}
							if (multiplayer != SINGLE)
							{
								for (node = stats[PLAYER_NUM]->inventory.first; node != nullptr; node = nextnode)
								{
									nextnode = node->next;
									Item* item = (Item*)node->element;
									if (itemCategory(item) == SPELL_CAT)
									{
										continue;    // don't drop spells on death, stupid!
									}
									list_RemoveNode(node);
								}
								stats[0]->helmet = NULL;
								stats[0]->breastplate = NULL;
								stats[0]->gloves = NULL;
								stats[0]->shoes = NULL;
								stats[0]->shield = NULL;
								stats[0]->weapon = NULL;
								stats[0]->cloak = NULL;
								stats[0]->amulet = NULL;
								stats[0]->ring = NULL;
								stats[0]->mask = NULL;
							}
						}
						else
						{
							// to not soft lock at Herx
							for ( node = stats[PLAYER_NUM]->inventory.first; node != nullptr; node = nextnode )
							{
								nextnode = node->next;
								Item* item = (Item*)node->element;
								if ( item->type == ARTIFACT_ORB_PURPLE )
								{
									int c = item->count;
									for ( c = item->count; c > 0; c-- )
									{
										entity = newEntity(-1, 1, map.entities, nullptr); //Item entity.
										entity->flags[INVISIBLE] = true;
										entity->flags[UPDATENEEDED] = true;
										entity->x = my->x;
										entity->y = my->y;
										entity->sizex = 4;
										entity->sizey = 4;
										entity->yaw = (rand() % 360) * (PI / 180.f);
										entity->vel_x = (rand() % 20 - 10) / 10.0;
										entity->vel_y = (rand() % 20 - 10) / 10.0;
										entity->vel_z = -.5;
										entity->flags[PASSABLE] = true;
										entity->flags[USERFLAG1] = true;
										entity->behavior = &actItem;
										entity->skill[10] = item->type;
										entity->skill[11] = item->status;
										entity->skill[12] = item->beatitude;
										entity->skill[13] = 1;
										entity->skill[14] = item->appearance;
										entity->skill[15] = item->identified;
									}
									break;
								}
							}
						}
						for ( node_t* mapNode = map.creatures->first; mapNode != nullptr; mapNode = mapNode->next )
						{
							Entity* mapCreature = (Entity*)mapNode->element;
							if ( mapCreature )
							{
								mapCreature->monsterEntityRenderAsTelepath = 0; // do a final pass to undo any telepath rendering.
							}
						}
					}
					else
					{
						if ( !(svFlags & SV_FLAG_KEEPINVENTORY) )
						{
							my->x = ((int)(my->x / 16)) * 16 + 8;
							my->y = ((int)(my->y / 16)) * 16 + 8;
							item = stats[PLAYER_NUM]->helmet;
							if (item)
							{
								int c = item->count;
								for (c = item->count; c > 0; c--)
								{
									dropItemMonster(item, my, stats[PLAYER_NUM]);
								}
							}
							item = stats[PLAYER_NUM]->breastplate;
							if (item)
							{
								int c = item->count;
								for (c = item->count; c > 0; c--)
								{
									dropItemMonster(item, my, stats[PLAYER_NUM]);
								}
							}
							item = stats[PLAYER_NUM]->gloves;
							if (item)
							{
								int c = item->count;
								for (c = item->count; c > 0; c--)
								{
									dropItemMonster(item, my, stats[PLAYER_NUM]);
								}
							}
							item = stats[PLAYER_NUM]->shoes;
							if (item)
							{
								int c = item->count;
								for (c = item->count; c > 0; c--)
								{
									dropItemMonster(item, my, stats[PLAYER_NUM]);
								}
							}
							item = stats[PLAYER_NUM]->shield;
							if (item)
							{
								int c = item->count;
								for (c = item->count; c > 0; c--)
								{
									dropItemMonster(item, my, stats[PLAYER_NUM]);
								}
							}
							item = stats[PLAYER_NUM]->weapon;
							if (item)
							{
								int c = item->count;
								for (c = item->count; c > 0; c--)
								{
									dropItemMonster(item, my, stats[PLAYER_NUM]);
								}
							}
							item = stats[PLAYER_NUM]->cloak;
							if (item)
							{
								int c = item->count;
								for (c = item->count; c > 0; c--)
								{
									dropItemMonster(item, my, stats[PLAYER_NUM]);
								}
							}
							item = stats[PLAYER_NUM]->amulet;
							if (item)
							{
								int c = item->count;
								for (c = item->count; c > 0; c--)
								{
									dropItemMonster(item, my, stats[PLAYER_NUM]);
								}
							}
							item = stats[PLAYER_NUM]->ring;
							if (item)
							{
								int c = item->count;
								for (c = item->count; c > 0; c--)
								{
									dropItemMonster(item, my, stats[PLAYER_NUM]);
								}
							}
							item = stats[PLAYER_NUM]->mask;
							if (item)
							{
								int c = item->count;
								for (c = item->count; c > 0; c--)
								{
									dropItemMonster(item, my, stats[PLAYER_NUM]);
								}
							}
							list_FreeAll(&stats[PLAYER_NUM]->inventory);
						}

						deleteMultiplayerSaveGames(); //Will only delete save games if was last player alive.
					}

					assailant[PLAYER_NUM] = false;
					assailantTimer[PLAYER_NUM] = 0;

					if ( multiplayer != SINGLE )
					{
						messagePlayer(PLAYER_NUM, language[578]);
					}
				}
				my->removeLightField();
				list_RemoveNode(my->mynode);
				return;
			}
		}
	}

	if ( PLAYER_NUM == clientnum && intro == false )
	{
		// effects of drunkenness
		if ( (stats[PLAYER_NUM]->EFFECTS[EFF_DRUNK] && (stats[PLAYER_NUM]->type != GOATMAN))
			|| stats[PLAYER_NUM]->EFFECTS[EFF_WITHDRAWAL] )
		{
			CHAR_DRUNK++;
			int drunkInterval = 180;
			if ( stats[PLAYER_NUM]->EFFECTS[EFF_WITHDRAWAL] )
			{
				if ( PLAYER_ALIVETIME < 800 )
				{
					drunkInterval = 300;
				}
				else
				{
					drunkInterval = TICKS_PER_SECOND * 30;
				}
			}

			if ( CHAR_DRUNK >= drunkInterval )
			{
				CHAR_DRUNK = 0;
				messagePlayer(PLAYER_NUM, language[579]);
				camera_shakex -= .04;
				camera_shakey -= 5;
			}
		}

		// calculate weight
		weight = 0;
		for ( node = stats[PLAYER_NUM]->inventory.first; node != NULL; node = node->next )
		{
			item = (Item*)node->element;
			if ( item != NULL )
			{
				if ( item->type >= 0 && item->type < NUMITEMS )
				{
					weight += items[item->type].weight * item->count;
				}
			}
		}
		weight += stats[PLAYER_NUM]->GOLD / 100;
		weightratio = (1000 + my->getSTR() * 100 - weight) / (double)(1000 + my->getSTR() * 100);
		weightratio = fmin(fmax(0, weightratio), 1);

		// calculate movement forces

		bool allowMovement = my->isMobile();
		bool pacified = stats[PLAYER_NUM]->EFFECTS[EFF_PACIFY];
		if ( !allowMovement && pacified )
		{
			if ( !stats[PLAYER_NUM]->EFFECTS[EFF_PARALYZED] && !stats[PLAYER_NUM]->EFFECTS[EFF_STUNNED]
				&& !stats[PLAYER_NUM]->EFFECTS[EFF_ASLEEP] )
			{
				allowMovement = true;
			}
		}

		if ( (!command || pacified) && allowMovement )
		{
			//x_force and y_force represent the amount of percentage pushed on that respective axis. Given a keyboard, it's binary; either you're pushing "move left" or you aren't. On an analog stick, it can range from whatever value to whatever.
			float x_force = 0;
			float y_force = 0;

			if ( pacified )
			{
				x_force = 0.f;
				y_force = -0.1;
				if ( stats[PLAYER_NUM]->EFFECTS[EFF_CONFUSED] )
				{
					y_force *= -1;
				}
			}
			else
			{
				if (!stats[PLAYER_NUM]->EFFECTS[EFF_CONFUSED])
				{
					//Normal controls.
					x_force = (*inputPressed(impulses[IN_RIGHT]) - *inputPressed(impulses[IN_LEFT]));
					y_force = (*inputPressed(impulses[IN_FORWARD]) - (double) * inputPressed(impulses[IN_BACK]) * .25);
					if ( noclip )
					{
						if ( keystatus[SDL_SCANCODE_LSHIFT] )
						{
							x_force = x_force * 0.5;
							y_force = y_force * 0.5;
						}
					}
				}
				else
				{
					//Confused controls.
					x_force = (*inputPressed(impulses[IN_LEFT]) - *inputPressed(impulses[IN_RIGHT]));
					y_force = (*inputPressed(impulses[IN_BACK]) - (double) * inputPressed(impulses[IN_FORWARD]) * .25);
				}

				if (game_controller && !*inputPressed(impulses[IN_LEFT]) && !*inputPressed(impulses[IN_RIGHT]))
				{
					x_force = game_controller->getLeftXPercent();

					if (stats[PLAYER_NUM]->EFFECTS[EFF_CONFUSED])
					{
						x_force *= -1;
					}
				}
				if (game_controller && !*inputPressed(impulses[IN_FORWARD]) && !*inputPressed(impulses[IN_BACK]))
				{
					y_force = game_controller->getLeftYPercent();

					if (stats[PLAYER_NUM]->EFFECTS[EFF_CONFUSED])
					{
						y_force *= -1;
					}

					if (y_force < 0)
					{
						y_force *= 0.25f;    //Move backwards more slowly.
					}
				}
			}


			real_t speedFactor = std::min((my->getDEX() * 0.1 + 15.5) * weightratio, 25 * 0.5 + 10);
			if ( my->getDEX() <= 5 )
			{
				speedFactor = std::min((my->getDEX() + 10) * weightratio, 25 * 0.5 + 10);
			}
			else if ( my->getDEX() <= 15 )
			{
				speedFactor = std::min((my->getDEX() * 0.2 + 14) * weightratio, 25 * 0.5 + 10);
			}
			PLAYER_VELX += y_force * cos(my->yaw) * .045 * speedFactor / (1 + (stats[PLAYER_NUM]->defending || stats[PLAYER_NUM]->sneaking == 1));
			PLAYER_VELY += y_force * sin(my->yaw) * .045 * speedFactor / (1 + (stats[PLAYER_NUM]->defending || stats[PLAYER_NUM]->sneaking == 1));
			PLAYER_VELX += x_force * cos(my->yaw + PI / 2) * .0225 * speedFactor / (1 + (stats[PLAYER_NUM]->defending || stats[PLAYER_NUM]->sneaking == 1));
			PLAYER_VELY += x_force * sin(my->yaw + PI / 2) * .0225 * speedFactor / (1 + (stats[PLAYER_NUM]->defending || stats[PLAYER_NUM]->sneaking == 1));
		}
		PLAYER_VELX *= .75;
		PLAYER_VELY *= .75;

		for ( node = map.creatures->first; node != nullptr; node = node->next ) //Since looking for players only, don't search full entity list. Best idea would be to directly example players[] though.
		{
			Entity* entity = (Entity*)node->element;
			if ( entity == my )
			{
				continue;
			}
			if ( entity->behavior == &actPlayer )
			{
				if ( entityInsideEntity(my, entity) )
				{
					double tangent = atan2(my->y - entity->y, my->x - entity->x);
					PLAYER_VELX += cos(tangent) * .075;
					PLAYER_VELY += sin(tangent) * .075;
				}
			}
		}

		// swimming slows you down
		bool amuletwaterbreathing = false;
		if ( stats[PLAYER_NUM]->amulet != NULL )
		{
			if ( stats[PLAYER_NUM]->amulet->type == AMULET_WATERBREATHING )
			{
				amuletwaterbreathing = true;
			}
		}
		if ( swimming && !amuletwaterbreathing )
		{
			PLAYER_VELX *= (((stats[PLAYER_NUM]->PROFICIENCIES[PRO_SWIMMING] / 100.f) * 50.f) + 50) / 100.f;
			PLAYER_VELY *= (((stats[PLAYER_NUM]->PROFICIENCIES[PRO_SWIMMING] / 100.f) * 50.f) + 50) / 100.f;

			if ( stats[PLAYER_NUM]->type == SKELETON )
			{
				if ( !swimDebuffMessageHasPlayed )
				{
					messagePlayer(PLAYER_NUM, language[3182]);
					swimDebuffMessageHasPlayed = true;
				}
				// no swim good
				PLAYER_VELX *= 0.5;
				PLAYER_VELY *= 0.5;
			}
		}

		// rotate
		if ( !command && my->isMobile() )
		{
			if ( !stats[PLAYER_NUM]->EFFECTS[EFF_CONFUSED] )
			{
				if ( noclip )
				{
					my->z -= (*inputPressed(impulses[IN_TURNR]) - *inputPressed(impulses[IN_TURNL])) * .25;
				}
				else
				{
					my->yaw += (*inputPressed(impulses[IN_TURNR]) - *inputPressed(impulses[IN_TURNL])) * .05;
				}
			}
			else
			{
				my->yaw += (*inputPressed(impulses[IN_TURNL]) - *inputPressed(impulses[IN_TURNR])) * .05;
			}
		}
		if ( shootmode && !gamePaused )
		{
			if ( !stats[PLAYER_NUM]->EFFECTS[EFF_CONFUSED] )
			{
				if ( smoothmouse )
				{
					if ( my->isMobile() )
					{
						PLAYER_ROTX += mousexrel * .006 * (mousespeed / 128.f);
					}
					PLAYER_ROTX = fmin(fmax(-0.35, PLAYER_ROTX), 0.35);
					PLAYER_ROTX *= .5;
				}
				else
				{
					if ( my->isMobile() )
					{
						PLAYER_ROTX = std::min<float>(std::max<float>(-0.35f, mousexrel * .01f * (mousespeed / 128.f)), 0.35f);
					}
					else
					{
						PLAYER_ROTX = 0;
					}
				}
			}
			else
			{
				if ( smoothmouse )
				{
					if ( my->isMobile() )
					{
						PLAYER_ROTX -= mousexrel * .006f * (mousespeed / 128.f);
					}
					PLAYER_ROTX = fmin(fmax(-0.35f, PLAYER_ROTX), 0.35f);
					PLAYER_ROTX *= .5;
				}
				else
				{
					if ( my->isMobile() )
					{
						PLAYER_ROTX = -std::min<float>(std::max<float>(-0.35f, mousexrel * .01f * (mousespeed / 128.f)), 0.35f);
					}
					else
					{
						PLAYER_ROTX = 0;
					}
				}
			}
		}
		my->yaw += PLAYER_ROTX;
		while ( my->yaw >= PI * 2 )
		{
			my->yaw -= PI * 2;
		}
		while ( my->yaw < 0 )
		{
			my->yaw += PI * 2;
		}
		if ( smoothmouse )
		{
			PLAYER_ROTX *= .5;
		}
		else
		{
			PLAYER_ROTX = 0;
		}

		// look up and down
		if ( !command && my->isMobile() )
		{
			if ( !stats[PLAYER_NUM]->EFFECTS[EFF_CONFUSED] )
			{
				my->pitch += (*inputPressed(impulses[IN_DOWN]) - *inputPressed(impulses[IN_UP])) * .05;
			}
			else
			{
				my->pitch += (*inputPressed(impulses[IN_UP]) - *inputPressed(impulses[IN_DOWN])) * .05;
			}
		}
		if ( shootmode && !gamePaused )
		{
			if ( !stats[PLAYER_NUM]->EFFECTS[EFF_CONFUSED] )
			{
				if ( smoothmouse )
				{
					if ( my->isMobile() )
					{
						PLAYER_ROTY += mouseyrel * .006 * (mousespeed / 128.f) * (reversemouse * 2 - 1);
					}
					PLAYER_ROTY = fmin(fmax(-0.35, PLAYER_ROTY), 0.35);
					PLAYER_ROTY *= .5;
				}
				else
				{
					if ( my->isMobile() )
					{
						PLAYER_ROTY = std::min<float>(std::max<float>(-0.35f, mouseyrel * .01f * (mousespeed / 128.f) * (reversemouse * 2 - 1)), 0.35f);
					}
					else
					{
						PLAYER_ROTY = 0;
					}
				}
			}
			else
			{
				if ( smoothmouse )
				{
					if ( my->isMobile() )
					{
						PLAYER_ROTY -= mouseyrel * .006f * (mousespeed / 128.f) * (reversemouse * 2 - 1);
					}
					PLAYER_ROTY = fmin(fmax(-0.35f, PLAYER_ROTY), 0.35f);
					PLAYER_ROTY *= .5;
				}
				else
				{
					if ( my->isMobile() )
					{
						PLAYER_ROTY = std::min<float>(std::max<float>(-0.35f, mouseyrel * .01f * (mousespeed / 128.f) * (reversemouse * 2 - 1)), 0.35f);
					}
					else
					{
						PLAYER_ROTY = 0;
					}
				}
			}
		}
		my->pitch -= PLAYER_ROTY;
		if ( softwaremode )
		{
			if ( my->pitch > PI / 6 )
			{
				my->pitch = PI / 6;
			}
			if ( my->pitch < -PI / 6 )
			{
				my->pitch = -PI / 6;
			}
		}
		else
		{
			if ( my->pitch > PI / 3 )
			{
				my->pitch = PI / 3;
			}
			if ( my->pitch < -PI / 3 )
			{
				my->pitch = -PI / 3;
			}
		}
		if ( !smoothmouse )
		{
			PLAYER_ROTY = 0;
		}
		else if ( !shootmode )
		{
			PLAYER_ROTY *= .5;
		}

		// send movement updates to server
		if ( multiplayer == CLIENT )
		{
			strcpy((char*)net_packet->data, "PMOV");
			net_packet->data[4] = clientnum;
			net_packet->data[5] = currentlevel;
			SDLNet_Write16((Sint16)(my->x * 32), &net_packet->data[6]);
			SDLNet_Write16((Sint16)(my->y * 32), &net_packet->data[8]);
			SDLNet_Write16((Sint16)(PLAYER_VELX * 128), &net_packet->data[10]);
			SDLNet_Write16((Sint16)(PLAYER_VELY * 128), &net_packet->data[12]);
			SDLNet_Write16((Sint16)(my->yaw * 128), &net_packet->data[14]);
			SDLNet_Write16((Sint16)(my->pitch * 128), &net_packet->data[16]);
			net_packet->data[18] = secretlevel;
			net_packet->address.host = net_server.host;
			net_packet->address.port = net_server.port;
			net_packet->len = 19;
			sendPacket(net_sock, -1, net_packet, 0);
		}

		// move
		if ( noclip == false )
		{
			// perform collision detection
			dist = clipMove(&my->x, &my->y, PLAYER_VELX, PLAYER_VELY, my);

			// bumping into monsters disturbs them
			if ( hit.entity && !everybodyfriendly )
			{
				if ( hit.entity->behavior == &actMonster )
				{
					bool enemy = my->checkEnemy(hit.entity);
					if ( enemy )
					{
						if ( hit.entity->monsterState == MONSTER_STATE_WAIT || (hit.entity->monsterState == MONSTER_STATE_HUNT && hit.entity->monsterTarget == 0) )
						{
							double tangent = atan2( my->y - hit.entity->y, my->x - hit.entity->x );
							hit.entity->skill[4] = 1;
							hit.entity->skill[6] = rand() % 10 + 1;
							hit.entity->fskill[4] = tangent;
						}
					}
				}
			}
		}
		else
		{
			// no collision detection
			my->x += PLAYER_VELX;
			my->y += PLAYER_VELY;
			dist = sqrt(PLAYER_VELX * PLAYER_VELX + PLAYER_VELY * PLAYER_VELY);
		}
	}

	if ( PLAYER_NUM != clientnum && multiplayer == SERVER )
	{
		// PLAYER_VEL* skills updated by messages sent to server from client

		// move (dead reckoning)
		if ( noclip == false )
		{
			dist = clipMove(&my->x, &my->y, PLAYER_VELX, PLAYER_VELY, my);

			// bumping into monsters disturbs them
			if ( hit.entity )
			{
				if ( hit.entity->behavior == &actMonster )
				{
					bool enemy = my->checkEnemy(hit.entity);
					if ( enemy )
					{
						if ( hit.entity->monsterState == MONSTER_STATE_WAIT || (hit.entity->monsterState == MONSTER_STATE_HUNT && hit.entity->monsterTarget == 0) )
						{
							double tangent = atan2( my->y - hit.entity->y, my->x - hit.entity->x );
							hit.entity->skill[4] = 1;
							hit.entity->skill[6] = rand() % 10 + 1;
							hit.entity->fskill[4] = tangent;
						}
					}
				}
			}
		}
		else
		{
			my->x += PLAYER_VELX;
			my->y += PLAYER_VELY;
			dist = sqrt(PLAYER_VELX * PLAYER_VELX + PLAYER_VELY * PLAYER_VELY);
		}
	}

	if ( PLAYER_NUM != clientnum && multiplayer == CLIENT )
	{
		dist = sqrt(PLAYER_VELX * PLAYER_VELX + PLAYER_VELY * PLAYER_VELY);
	}

	if ( PLAYER_NUM == clientnum && ticks % 65 == 0 )
	{
		for ( node_t* mapNode = map.creatures->first; mapNode != nullptr; mapNode = mapNode->next )
		{
			Entity* mapCreature = (Entity*)mapNode->element;
			if ( mapCreature )
			{
				if ( stats[PLAYER_NUM]->EFFECTS[EFF_TELEPATH] )
				{
					// periodically set the telepath rendering flag.
					mapCreature->monsterEntityRenderAsTelepath = 1;
				}
				else
				{
					mapCreature->monsterEntityRenderAsTelepath = 0;
				}
			}
		}
	}

	// move bodyparts
	for (bodypart = 0, node = my->children.first; node != NULL; node = node->next, bodypart++)
	{
		if (bodypart == 0)
		{
			// hudweapon case
			continue;
		}
		entity = (Entity*)node->element;
		entity->x = my->x;
		entity->y = my->y;
		entity->z = my->z;

		if ( bodypart < 9 ) // don't shift helm/mask. 
		{
			// these monsters are shorter than humans so extend the limbs down to floor, gives longer neck.
			if ( playerRace == GOBLIN || playerRace == INSECTOID || playerRace == GOATMAN )
			{
				entity->z += 0.5;
			}
			else if ( playerRace == SKELETON || playerRace == AUTOMATON )
			{
				entity->z += 0.25;
			}
		}

		entity->yaw = my->yaw;
		if ( bodypart == 2 || bodypart == 5 ) // right leg, left arm
		{
			if ( bodypart == 2 )
			{
				rightbody = (Entity*)node->next->element;
			}
			if ( bodypart == 5 )
			{
				shieldarm = entity;
			}
			node_t* shieldNode = list_Node(&my->children, 7);
			if ( shieldNode )
			{
				Entity* shield = (Entity*)shieldNode->element;
				if ( (fabs(PLAYER_VELX) > 0.1 || fabs(PLAYER_VELY) > 0.1) && (bodypart != 5 || shield->flags[INVISIBLE]) )
				{
					if ( !rightbody->skill[0] )
					{
						entity->pitch -= dist * PLAYERWALKSPEED;
						if ( entity->pitch < -PI / 4.0 )
						{
							entity->pitch = -PI / 4.0;
							if (bodypart == 2 && dist > .4 && !levitating && !swimming)
							{
								node_t* tempNode = list_Node(&my->children, 2);
								if ( tempNode )
								{
									Entity* foot = (Entity*)tempNode->element;
									if ( foot->sprite == 152 || foot->sprite == 153 )
									{
										playSoundEntityLocal(my, 7 + rand() % 7, 32);
									}
									else if ( foot->sprite == 156 || foot->sprite == 157 )
									{
										playSoundEntityLocal(my, 14 + rand() % 7, 32);
									}
									else
									{
										playSoundEntityLocal(my, rand() % 7, 32);
									}
								}
							}
						}
					}
					else
					{
						entity->pitch += dist * PLAYERWALKSPEED;
						if ( entity->pitch > PI / 4.0 )
						{
							entity->pitch = PI / 4.0;
							if (bodypart == 2 && dist > .4 && !levitating && !swimming)
							{
								node_t* tempNode = list_Node(&my->children, 2);
								if ( tempNode )
								{
									Entity* foot = (Entity*)tempNode->element;
									if ( foot->sprite == 152 || foot->sprite == 153 )
									{
										playSoundEntityLocal(my, 7 + rand() % 7, 32);
									}
									else if ( foot->sprite == 156 || foot->sprite == 157 )
									{
										playSoundEntityLocal(my, 14 + rand() % 7, 32);
									}
									else
									{
										playSoundEntityLocal(my, rand() % 7, 32);
									}
								}
							}
						}
					}
				}
				else
				{
					if ( entity->pitch < 0 )
					{
						entity->pitch += 1 / fmax(dist * .1, 10.0);
						if ( entity->pitch > 0 )
						{
							entity->pitch = 0;
						}
					}
					else if ( entity->pitch > 0 )
					{
						entity->pitch -= 1 / fmax(dist * .1, 10.0);
						if ( entity->pitch < 0 )
						{
							entity->pitch = 0;
						}
					}
				}
			}
		}
		else if ( bodypart == 3 || bodypart == 4 || bodypart == 8 ) // left leg, right arm, cloak
		{
			if ( bodypart == 4 )
			{
				weaponarm = entity;
				if ( PLAYER_ATTACK == 1 )
				{
					// vertical chop
					if ( PLAYER_ATTACKTIME == 0 )
					{
						PLAYER_ARMBENDED = 0;
						PLAYER_WEAPONYAW = 0;
						entity->pitch = 0;
						entity->roll = 0;
						entity->skill[1] = 0;
					}
					else
					{
						if ( entity->skill[1] == 0 )
						{
							// upswing
							if ( limbAnimateToLimit(entity, ANIMATE_PITCH, -0.5, 5 * PI / 4, false, 0.0) )
							{
								entity->skill[1] = 1;
							}
						}
						else
						{
							if ( entity->pitch >= 3 * PI / 2 )
							{
								PLAYER_ARMBENDED = 1;
							}
							if ( limbAnimateToLimit(entity, ANIMATE_PITCH, 0.3, PI / 4, false, 0.0) )
							{
								entity->skill[0] = rightbody->skill[0];
								entity->skill[1] = 0;
								PLAYER_WEAPONYAW = 0;
								entity->pitch = rightbody->pitch;
								entity->roll = 0;
								PLAYER_ARMBENDED = 0;
								PLAYER_ATTACK = 0;
							}
						}
					}
				}
				else if ( PLAYER_ATTACK == 2 )
				{
					// horizontal chop
					if ( PLAYER_ATTACKTIME == 0 )
					{
						PLAYER_ARMBENDED = 1;
						PLAYER_WEAPONYAW = -3 * PI / 4;
						entity->pitch = 0;
						entity->roll = -PI / 2;
					}
					else
					{
						if ( PLAYER_WEAPONYAW >= PI / 8 )
						{
							entity->skill[0] = rightbody->skill[0];
							PLAYER_WEAPONYAW = 0;
							entity->pitch = rightbody->pitch;
							entity->roll = 0;
							PLAYER_ARMBENDED = 0;
							PLAYER_ATTACK = 0;
						}
						else
						{
							PLAYER_WEAPONYAW += .25;
						}
					}
				}
				else if ( PLAYER_ATTACK == 3 )
				{
					// stab
					if ( PLAYER_ATTACKTIME == 0 )
					{
						PLAYER_ARMBENDED = 0;
						PLAYER_WEAPONYAW = 0;
						entity->pitch = 0;
						entity->roll = 0;
					}
					else
					{
						if ( PLAYER_ATTACKTIME >= 5 )
						{
							PLAYER_ARMBENDED = 1;
							limbAnimateToLimit(entity, ANIMATE_PITCH, -0.5, 11 * PI / 6, false, 0.0);
						}
						else
						{
							limbAnimateToLimit(entity, ANIMATE_PITCH, 0.4, 2 * PI / 3, false, 0.0);
						}
						if ( PLAYER_ATTACKTIME >= 10 )
						{
							entity->skill[0] = rightbody->skill[0];
							PLAYER_WEAPONYAW = 0;
							entity->pitch = rightbody->pitch;
							entity->roll = 0;
							PLAYER_ARMBENDED = 0;
							PLAYER_ATTACK = 0;
						}
					}
				}
			}
			else if ( bodypart == 8 )
			{
				entity->pitch = entity->fskill[0];
			}

			if ( bodypart != 4 || (PLAYER_ATTACK == 0 && PLAYER_ATTACKTIME == 0) )
			{
				if ( fabs(PLAYER_VELX) > 0.1 || fabs(PLAYER_VELY) > 0.1 )
				{
					if ( entity->skill[0] )
					{
						entity->pitch -= dist * PLAYERWALKSPEED;
						if ( entity->pitch < -PI / 4.0 )
						{
							entity->skill[0] = 0;
							entity->pitch = -PI / 4.0;
						}
					}
					else
					{
						entity->pitch += dist * PLAYERWALKSPEED;
						if ( entity->pitch > PI / 4.0 )
						{
							entity->skill[0] = 1;
							entity->pitch = PI / 4.0;
						}
					}
				}
				else
				{
					if ( entity->pitch < 0 )
					{
						entity->pitch += 1 / fmax(dist * .1, 10.0);
						if ( entity->pitch > 0 )
						{
							entity->pitch = 0;
						}
					}
					else if ( entity->pitch > 0 )
					{
						entity->pitch -= 1 / fmax(dist * .1, 10.0);
						if ( entity->pitch < 0 )
						{
							entity->pitch = 0;
						}
					}
				}
			}
			if ( bodypart == 8 )
			{
				entity->fskill[0] = entity->pitch;
				entity->roll = my->roll - fabs(entity->pitch) / 2;
				entity->pitch = 0;
			}
		}

		switch ( bodypart )
		{
			// torso
			case 1:
				torso = entity;
				entity->focalx = limbs[playerRace][1][0];
				entity->focaly = limbs[playerRace][1][1];
				entity->focalz = limbs[playerRace][1][2];
				if ( multiplayer != CLIENT )
				{
					if ( stats[PLAYER_NUM]->breastplate == NULL )
					{
						entity->setDefaultPlayerModel(PLAYER_NUM, playerRace, LIMB_HUMANOID_TORSO);
					}
					else
					{
						entity->sprite = itemModel(stats[PLAYER_NUM]->breastplate);
					}
					if ( multiplayer == SERVER )
					{
						// update sprites for clients
						if ( entity->skill[10] != entity->sprite )
						{
							entity->skill[10] = entity->sprite;
							serverUpdateEntityBodypart(my, bodypart);
						}
						if ( PLAYER_ALIVETIME == TICKS_PER_SECOND + bodypart )
						{
							serverUpdateEntityBodypart(my, bodypart);
						}
					}
				}
				my->setHumanoidLimbOffset(entity, playerRace, LIMB_HUMANOID_TORSO);
				break;
			// right leg
			case 2:
				if ( multiplayer != CLIENT )
				{
					if ( stats[PLAYER_NUM]->shoes == NULL )
					{
						entity->setDefaultPlayerModel(PLAYER_NUM, playerRace, LIMB_HUMANOID_RIGHTLEG);
					}
					else
					{
						my->setBootSprite(entity, SPRITE_BOOT_RIGHT_OFFSET);
					}
					if ( multiplayer == SERVER )
					{
						// update sprites for clients
						if ( entity->skill[10] != entity->sprite )
						{
							entity->skill[10] = entity->sprite;
							serverUpdateEntityBodypart(my, bodypart);
						}
						if ( PLAYER_ALIVETIME == TICKS_PER_SECOND + bodypart )
						{
							serverUpdateEntityBodypart(my, bodypart);
						}
					}
				}
				my->setHumanoidLimbOffset(entity, playerRace, LIMB_HUMANOID_RIGHTLEG);
				break;
			// left leg
			case 3:
				if ( multiplayer != CLIENT )
				{
					if ( stats[PLAYER_NUM]->shoes == NULL )
					{
						entity->setDefaultPlayerModel(PLAYER_NUM, playerRace, LIMB_HUMANOID_LEFTLEG);
					}
					else
					{
						my->setBootSprite(entity, SPRITE_BOOT_LEFT_OFFSET);
					}
					if ( multiplayer == SERVER )
					{
						// update sprites for clients
						if ( entity->skill[10] != entity->sprite )
						{
							entity->skill[10] = entity->sprite;
							serverUpdateEntityBodypart(my, bodypart);
						}
						if ( PLAYER_ALIVETIME == TICKS_PER_SECOND + bodypart )
						{
							serverUpdateEntityBodypart(my, bodypart);
						}
					}
				}
				my->setHumanoidLimbOffset(entity, playerRace, LIMB_HUMANOID_LEFTLEG);
				break;
			// right arm
			case 4:
			{
				if ( multiplayer != CLIENT )
				{
					if ( stats[PLAYER_NUM]->gloves == NULL )
					{
						entity->setDefaultPlayerModel(PLAYER_NUM, playerRace, LIMB_HUMANOID_RIGHTARM);
					}
					else
					{
						if ( setGloveSprite(stats[PLAYER_NUM], entity, SPRITE_GLOVE_RIGHT_OFFSET) != 0 )
						{
							// successfully set sprite for the human model
						}
					}
					if ( !PLAYER_ARMBENDED )
					{
						entity->sprite += 2 * (stats[PLAYER_NUM]->weapon != NULL);
					}
					if ( multiplayer == SERVER )
					{
						// update sprites for clients
						if ( entity->skill[10] != entity->sprite )
						{
							entity->skill[10] = entity->sprite;
							serverUpdateEntityBodypart(my, bodypart);
						}
						if ( PLAYER_ALIVETIME == TICKS_PER_SECOND + bodypart )
						{
							serverUpdateEntityBodypart(my, bodypart);
						}
					}
				}
				my->setHumanoidLimbOffset(entity, playerRace, LIMB_HUMANOID_RIGHTARM);
				node_t* tempNode = list_Node(&my->children, 6);
				if ( tempNode )
				{
					Entity* weapon = (Entity*)tempNode->element;
					if ( weapon->flags[INVISIBLE] || PLAYER_ARMBENDED )
					{
						if ( playerRace == INCUBUS || playerRace == SUCCUBUS )
						{
							entity->focalx = limbs[playerRace][4][0] - 0.25;
							entity->focaly = limbs[playerRace][4][1] - 0.25;
							entity->focalz = limbs[playerRace][4][2];
						}
						else
						{
							entity->focalx = limbs[playerRace][4][0]; // 0
							entity->focaly = limbs[playerRace][4][1]; // 0
							entity->focalz = limbs[playerRace][4][2]; // 1.5
						}
					}
					else
					{
						if ( playerRace == INCUBUS || playerRace == SUCCUBUS )
						{
							entity->focalx = limbs[playerRace][4][0];
							entity->focaly = limbs[playerRace][4][1];
							entity->focalz = limbs[playerRace][4][2];
						}
						else
						{
							entity->focalx = limbs[playerRace][4][0] + 0.75;
							entity->focaly = limbs[playerRace][4][1];
							entity->focalz = limbs[playerRace][4][2] - 0.75;
						}
					}
				}
				entity->yaw += PLAYER_WEAPONYAW;
				break;
			}
			// left arm
			case 5:
			{
				if ( multiplayer != CLIENT )
				{
					if ( stats[PLAYER_NUM]->gloves == NULL )
					{
						entity->setDefaultPlayerModel(PLAYER_NUM, playerRace, LIMB_HUMANOID_LEFTARM);
					}
					else
					{
						if ( setGloveSprite(stats[PLAYER_NUM], entity, SPRITE_GLOVE_LEFT_OFFSET) != 0 )
						{
							// successfully set sprite for the human model
						}
					}
					entity->sprite += 2 * (stats[PLAYER_NUM]->shield != NULL);
					if ( multiplayer == SERVER )
					{
						// update sprites for clients
						if ( entity->skill[10] != entity->sprite )
						{
							entity->skill[10] = entity->sprite;
							serverUpdateEntityBodypart(my, bodypart);
						}
						if ( PLAYER_ALIVETIME == TICKS_PER_SECOND + bodypart )
						{
							serverUpdateEntityBodypart(my, bodypart);
						}
					}
				}
				my->setHumanoidLimbOffset(entity, playerRace, LIMB_HUMANOID_LEFTARM);
				node_t* tempNode = list_Node(&my->children, 7);
				if ( tempNode )
				{
					Entity* shield = (Entity*)tempNode->element;
					if ( shield->flags[INVISIBLE] )
					{
						if ( playerRace == INCUBUS || playerRace == SUCCUBUS )
						{
							entity->focalx = limbs[playerRace][5][0] - 0.25;
							entity->focaly = limbs[playerRace][5][1] + 0.25;
							entity->focalz = limbs[playerRace][5][2];
						}
						else
						{
							entity->focalx = limbs[playerRace][5][0]; // 0
							entity->focaly = limbs[playerRace][5][1]; // 0
							entity->focalz = limbs[playerRace][5][2]; // 1.5
						}
					}
					else
					{
						if ( playerRace == INCUBUS || playerRace == SUCCUBUS )
						{
							entity->focalx = limbs[playerRace][5][0];
							entity->focaly = limbs[playerRace][5][1];
							entity->focalz = limbs[playerRace][5][2];
						}
						else
						{
							entity->focalx = limbs[playerRace][5][0] + 0.75;
							entity->focaly = limbs[playerRace][5][1];
							entity->focalz = limbs[playerRace][5][2] - 0.75;
						}
					}
				}
				if ( multiplayer != CLIENT )
				{
					real_t prevYaw = PLAYER_SHIELDYAW;
					if ( stats[PLAYER_NUM]->defending )
					{
						PLAYER_SHIELDYAW = PI / 5;
					}
					else
					{
						PLAYER_SHIELDYAW = 0;
					}
					if ( prevYaw != PLAYER_SHIELDYAW || ticks % 200 == 0 )
					{
						serverUpdateEntityFSkill(players[PLAYER_NUM]->entity, 8);
					}
				}
				entity->yaw += PLAYER_SHIELDYAW;
				break;
			}
			// weapon
			case 6:
				if ( multiplayer != CLIENT )
				{
					if ( swimming )
					{
						entity->flags[INVISIBLE] = true;
					}
					else
					{
						if ( stats[PLAYER_NUM]->weapon == NULL || my->isInvisible() )
						{
							entity->flags[INVISIBLE] = true;
						}
						else
						{
							entity->sprite = itemModel(stats[PLAYER_NUM]->weapon);
							if ( itemCategory(stats[PLAYER_NUM]->weapon) == SPELLBOOK )
							{
								entity->flags[INVISIBLE] = true;
							}
							else
							{
								entity->flags[INVISIBLE] = false;
							}
						}
					}
					if ( multiplayer == SERVER )
					{
						// update sprites for clients
						if ( entity->skill[10] != entity->sprite )
						{
							entity->skill[10] = entity->sprite;
							serverUpdateEntityBodypart(my, bodypart);
						}
						if ( entity->skill[11] != entity->flags[INVISIBLE] )
						{
							entity->skill[11] = entity->flags[INVISIBLE];
							serverUpdateEntityBodypart(my, bodypart);
						}
						if ( PLAYER_ALIVETIME == TICKS_PER_SECOND + bodypart )
						{
							serverUpdateEntityBodypart(my, bodypart);
						}
					}
				}
				else
				{
					if ( entity->sprite <= 0 )
					{
						entity->flags[INVISIBLE] = true;
					}
				}
				my->handleHumanoidWeaponLimb(entity, weaponarm);
				break;
			// shield
			case 7:
				if ( multiplayer != CLIENT )
				{
					if ( swimming )
					{
						entity->flags[INVISIBLE] = true;
					}
					else
					{
						if ( stats[PLAYER_NUM]->shield == NULL )
						{
							entity->flags[INVISIBLE] = true;
							entity->sprite = 0;
						}
						else
						{
							entity->flags[INVISIBLE] = false;
							entity->sprite = itemModel(stats[PLAYER_NUM]->shield);
						}
						if ( my->isInvisible() )
						{
							entity->flags[INVISIBLE] = true;
						}
					}
					if ( multiplayer == SERVER )
					{
						// update sprites for clients
						if ( entity->skill[10] != entity->sprite )
						{
							entity->skill[10] = entity->sprite;
							serverUpdateEntityBodypart(my, bodypart);
						}
						if ( entity->skill[11] != entity->flags[INVISIBLE] )
						{
							entity->skill[11] = entity->flags[INVISIBLE];
							serverUpdateEntityBodypart(my, bodypart);
						}
						if ( PLAYER_ALIVETIME == TICKS_PER_SECOND + bodypart )
						{
							serverUpdateEntityBodypart(my, bodypart);
						}
					}
				}
				else
				{
					if ( entity->sprite <= 0 )
					{
						entity->flags[INVISIBLE] = true;
					}
				}
				my->handleHumanoidShieldLimb(entity, shieldarm);
				break;
			// cloak
			case 8:
				entity->focalx = limbs[playerRace][8][0];
				entity->focaly = limbs[playerRace][8][1];
				entity->focalz = limbs[playerRace][8][2];
				entity->scalex = 1.01;
				entity->scaley = 1.01;
				if ( multiplayer != CLIENT )
				{
					if ( stats[PLAYER_NUM]->cloak == NULL || my->isInvisible() )
					{
						entity->flags[INVISIBLE] = true;
					}
					else
					{
						entity->flags[INVISIBLE] = false;
						entity->sprite = itemModel(stats[PLAYER_NUM]->cloak);
					}
					if ( multiplayer == SERVER )
					{
						// update sprites for clients
						if ( entity->skill[10] != entity->sprite )
						{
							entity->skill[10] = entity->sprite;
							serverUpdateEntityBodypart(my, bodypart);
						}
						if ( entity->skill[11] != entity->flags[INVISIBLE] )
						{
							entity->skill[11] = entity->flags[INVISIBLE];
							serverUpdateEntityBodypart(my, bodypart);
						}
						if ( PLAYER_ALIVETIME == TICKS_PER_SECOND + bodypart )
						{
							serverUpdateEntityBodypart(my, bodypart);
						}
					}
				}
				else
				{
					if ( entity->sprite <= 0 )
					{
						entity->flags[INVISIBLE] = true;
					}
				}

				if ( entity->sprite == items[CLOAK_BACKPACK].index )
				{
					// human
					if ( playerRace == HUMAN || playerRace == VAMPIRE )
					{
						entity->focaly = limbs[playerRace][8][1] + 0.25;
						entity->focalz = limbs[playerRace][8][2] - 0.3;
					}
					else if ( playerRace == SUCCUBUS || playerRace == INCUBUS )
					{
						// succubus/incubus
						entity->focaly = limbs[playerRace][8][1] + 0.25;
						entity->focalz = limbs[playerRace][8][2] - 0.7;
					}
					else if ( playerRace == SKELETON )
					{
						entity->focaly = limbs[playerRace][8][1] + 0.25;
						entity->focalz = limbs[playerRace][8][2] - 0.5;
					}
					else if ( playerRace == AUTOMATON )
					{
						entity->focaly = limbs[playerRace][8][1] - 0.25;
						entity->focalz = limbs[playerRace][8][2] - 0.5;
					}
					else if ( playerRace == GOATMAN || playerRace == INSECTOID || playerRace == GOBLIN )
					{
						entity->focaly = limbs[playerRace][8][1] - 0.25;
						entity->focalz = limbs[playerRace][8][2] - 0.5;
					}

					entity->scalex = 0.99;
					entity->scaley = 0.99;
				}
				entity->x -= cos(my->yaw);
				entity->y -= sin(my->yaw);
				entity->yaw += PI / 2;
				break;
			// helm
			case 9:
				entity->focalx = limbs[playerRace][9][0]; // 0
				entity->focaly = limbs[playerRace][9][1]; // 0
				entity->focalz = limbs[playerRace][9][2]; // -1.75
				entity->pitch = my->pitch;
				entity->roll = 0;
				if ( multiplayer != CLIENT )
				{
					entity->sprite = itemModel(stats[PLAYER_NUM]->helmet);
					if ( stats[PLAYER_NUM]->helmet == NULL || my->isInvisible() )
					{
						entity->flags[INVISIBLE] = true;
					}
					else
					{
						entity->flags[INVISIBLE] = false;
					}
					if ( multiplayer == SERVER )
					{
						// update sprites for clients
						if ( entity->skill[10] != entity->sprite )
						{
							entity->skill[10] = entity->sprite;
							serverUpdateEntityBodypart(my, bodypart);
						}
						if ( entity->skill[11] != entity->flags[INVISIBLE] )
						{
							entity->skill[11] = entity->flags[INVISIBLE];
							serverUpdateEntityBodypart(my, bodypart);
						}
						if ( PLAYER_ALIVETIME == TICKS_PER_SECOND + bodypart )
						{
							serverUpdateEntityBodypart(my, bodypart);
						}
					}
				}
				else
				{
					if ( entity->sprite <= 0 )
					{
						entity->flags[INVISIBLE] = true;
					}
				}
				my->setHelmetLimbOffset(entity);
				break;
			// mask
			case 10:
				entity->focalx = limbs[playerRace][10][0]; // 0
				entity->focaly = limbs[playerRace][10][1]; // 0
				entity->focalz = limbs[playerRace][10][2]; // .5
				entity->pitch = my->pitch;
				entity->roll = PI / 2;
				if ( multiplayer != CLIENT )
				{
					bool hasSteelHelm = false;
					if ( stats[PLAYER_NUM]->helmet )
						if ( stats[PLAYER_NUM]->helmet->type == STEEL_HELM )
						{
							hasSteelHelm = true;
						}
					if ( stats[PLAYER_NUM]->mask == NULL || my->isInvisible() || hasSteelHelm )
					{
						entity->flags[INVISIBLE] = true;
					}
					else
					{
						entity->flags[INVISIBLE] = false;
					}
					if ( stats[PLAYER_NUM]->mask != NULL )
					{
						if ( stats[PLAYER_NUM]->mask->type == TOOL_GLASSES )
						{
							entity->sprite = 165; // GlassesWorn.vox
						}
						else
						{
							entity->sprite = itemModel(stats[PLAYER_NUM]->mask);
						}
					}
					if ( multiplayer == SERVER )
					{
						// update sprites for clients
						if ( entity->skill[10] != entity->sprite )
						{
							entity->skill[10] = entity->sprite;
							serverUpdateEntityBodypart(my, bodypart);
						}
						if ( entity->skill[11] != entity->flags[INVISIBLE] )
						{
							entity->skill[11] = entity->flags[INVISIBLE];
							serverUpdateEntityBodypart(my, bodypart);
						}
						if ( PLAYER_ALIVETIME == TICKS_PER_SECOND + bodypart )
						{
							serverUpdateEntityBodypart(my, bodypart);
						}
					}
				}
				else
				{
					if ( entity->sprite <= 0 )
					{
						entity->flags[INVISIBLE] = true;
					}
				}
				if ( entity->sprite == 165 )
				{
					entity->focalx = limbs[playerRace][10][0] + .25; // .25
					entity->focaly = limbs[playerRace][10][1] - 2.25; // -2.25
					entity->focalz = limbs[playerRace][10][2]; // .5
				}
				else
				{
					entity->focalx = limbs[playerRace][10][0] + .35; // .35
					entity->focaly = limbs[playerRace][10][1] - 2; // -2
					entity->focalz = limbs[playerRace][10][2]; // .5
				}
				break;
			case 11:
				additionalLimb = entity;
				entity->focalx = limbs[playerRace][11][0];
				entity->focaly = limbs[playerRace][11][1];
				entity->focalz = limbs[playerRace][11][2];
				entity->flags[INVISIBLE] = true;
				if ( playerRace == INSECTOID )
				{
					entity->flags[INVISIBLE] = my->flags[INVISIBLE];
					if ( stats[PLAYER_NUM]->sex == FEMALE )
					{
						entity->sprite = 771;
					}
					else
					{
						entity->sprite = 750;
					}
					if ( torso && torso->sprite != 727 && torso->sprite != 761 && torso->sprite != 458 )
					{
						// wearing armor, offset more.
						entity->x -= 2.25 * cos(my->yaw);
						entity->y -= 2.25 * sin(my->yaw);
					}
					else
					{
						entity->x -= 1.5 * cos(my->yaw);
						entity->y -= 1.5 * sin(my->yaw);
					}
					bool moving = false;
					if ( fabs(PLAYER_VELX) > 0.1 || fabs(PLAYER_VELY) > 0.1 )
					{
						moving = true;
					}

					if ( entity->skill[0] == 0 )
					{
						if ( moving )
						{
							entity->fskill[0] += std::min(dist * PLAYERWALKSPEED, 2.f * PLAYERWALKSPEED); // move proportional to move speed
						}
						else if ( PLAYER_ATTACK != 0 )
						{
							entity->fskill[0] += PLAYERWALKSPEED; // move fixed speed when attacking if stationary
						}
						else
						{
							entity->fskill[0] += 0.01; // otherwise move slow idle
						}

						if ( entity->fskill[0] > PI / 3 || ((!moving || PLAYER_ATTACK != 0) && entity->fskill[0] > PI / 5) ) 
						{
							// switch direction if angle too great, angle is shorter if attacking or stationary
							entity->skill[0] = 1;
						}
					}
					else // reverse of the above
					{
						if ( moving )
						{
							entity->fskill[0] -= std::min(dist * PLAYERWALKSPEED, 2.f * PLAYERWALKSPEED);
						}
						else if ( PLAYER_ATTACK != 0 )
						{
							entity->fskill[0] -= PLAYERWALKSPEED;
						}
						else
						{
							entity->fskill[0] -= 0.007;
						}

						if ( entity->fskill[0] < -PI / 32 )
						{
							entity->skill[0] = 0;
						}
					}
					entity->yaw += entity->fskill[0];
				}
				break;
			case 12:
				entity->focalx = limbs[playerRace][12][0];
				entity->focaly = limbs[playerRace][12][1];
				entity->focalz = limbs[playerRace][12][2];
				entity->flags[INVISIBLE] = true;
				if ( playerRace == INSECTOID )
				{
					entity->flags[INVISIBLE] = my->flags[INVISIBLE];
					if ( stats[PLAYER_NUM]->sex == FEMALE )
					{
						entity->sprite = 772;
					}
					else
					{
						entity->sprite = 751;
					}
					if ( additionalLimb ) // follow the yaw of the previous limb.
					{
						entity->yaw -= additionalLimb->fskill[0];
					}
					if ( torso && torso->sprite != 727 && torso->sprite != 761 && torso->sprite != 458 )
					{
						// wearing armor, offset more.
						entity->x -= 2.25 * cos(my->yaw);
						entity->y -= 2.25 * sin(my->yaw);
					}
					else
					{
						entity->x -= 1.5 * cos(my->yaw);
						entity->y -= 1.5 * sin(my->yaw);
					}
				}
				break;
			default:
				break;
		}
	}
	// rotate shield a bit
	node_t* shieldNode = list_Node(&my->children, 7);
	if ( shieldNode )
	{
		Entity* shieldEntity = (Entity*)shieldNode->element;
		if ( shieldEntity->sprite != items[TOOL_TORCH].index && shieldEntity->sprite != items[TOOL_LANTERN].index && shieldEntity->sprite != items[TOOL_CRYSTALSHARD].index )
		{
			shieldEntity->yaw -= PI / 6;
		}
	}
	if ( PLAYER_ATTACK != 0 )
	{
		PLAYER_ATTACKTIME++;
	}
	else
	{
		PLAYER_ATTACKTIME = 0;
	}

	if ( PLAYER_NUM == clientnum && intro == false )
	{
		// camera
		if ( !PLAYER_DEBUGCAM )
		{
			camera.x = my->x / 16.0;
			camera.y = my->y / 16.0;
			camera.z = (my->z * 2) + PLAYER_BOB - 2.5;
			camera.ang = my->yaw;
			if ( softwaremode )
			{
				camera.vang = (my->pitch / (PI / 4)) * camera.winh;
			}
			else
			{
				camera.vang = my->pitch;
			}
		}
	}
}

// client function
void actPlayerLimb(Entity* my)
{
	int i;

	Entity* parent = uidToEntity(my->parent);

	if ( multiplayer == CLIENT )
	{
		if ( stats[PLAYER_NUM]->HP <= 0 )
		{
			my->flags[INVISIBLE] = true;
			return;
		}
	}

	if ( multiplayer != CLIENT )
	{
		for ( i = 0; i < MAXPLAYERS; i++ )
		{
			if ( inrange[i] )
			{
				if ( i == 0 && selectedEntity == my )
				{
					parent->skill[14] = i + 1;
				}
				else if ( client_selected[i] == my )
				{
					parent->skill[14] = i + 1;
				}
			}
		}
	}

	if ( parent && parent->monsterEntityRenderAsTelepath == 1 )
	{
		my->monsterEntityRenderAsTelepath = 1;
	}
	else
	{
		my->monsterEntityRenderAsTelepath = 0;
	}

	if (multiplayer != CLIENT)
	{
		return;
	}

	if (my->skill[2] < 0 || my->skill[2] >= MAXPLAYERS )
	{
		return;
	}
	if (players[my->skill[2]] == nullptr || players[my->skill[2]]->entity == nullptr)
	{
		list_RemoveNode(my->mynode);
		return;
	}

	//TODO: These three are _NOT_ PLAYERSWAP
	//my->vel_x = players[my->skill[2]]->vel_x;
	//my->vel_y = players[my->skill[2]]->vel_y;
	//my->vel_z = players[my->skill[2]]->vel_z;

	// set light size
	if (my->sprite == 93)   // torch
	{
		my->skill[4] = 1;
		players[my->skill[2]]->entity->skill[1] = 6;
	}
	else if (my->sprite == 94)     // lantern
	{
		my->skill[4] = 1;
		players[my->skill[2]]->entity->skill[1] = 9;
	}
	else if ( my->sprite == 529 )	// crystal shard
	{
		my->skill[4] = 1;
		players[my->skill[2]]->entity->skill[1] = 4;
	}
	else
	{
		if (my->skill[4] == 1)
		{
			players[my->skill[2]]->entity->skill[1] = 0;
		}
	}

}

void Entity::playerLevelEntrySpeechSecond()
{
	int timeDiff = playerAliveTime - 300;
	int orangeSpeechVolume = 128;
	int blueSpeechVolume = 112;
	if ( timeDiff > 0 && playerLevelEntrySpeech > 0 && !secretlevel )
	{
		switch ( currentlevel )
		{
			case 26:
				switch ( playerLevelEntrySpeech )
				{
					case 1:
						if ( timeDiff == 200 )
						{
							playSound(342, orangeSpeechVolume);
							messagePlayerColor(clientnum, uint32ColorOrange(*mainsurface), language[2616]);
							playerLevelEntrySpeech = 0;
						}
						break;
					case 2:
						if ( timeDiff == 200 )
						{
							playSound(344, blueSpeechVolume);
							messagePlayerColor(clientnum, uint32ColorBaronyBlue(*mainsurface), language[2618]);
						}
						else if ( timeDiff == 350 )
						{
							playSound(345, orangeSpeechVolume);
							messagePlayerColor(clientnum, uint32ColorOrange(*mainsurface), language[2619]);
							playerLevelEntrySpeech = 0;
						}
						break;
					case 3:
						if ( timeDiff == 200 )
						{
							playSound(347, blueSpeechVolume);
							messagePlayerColor(clientnum, uint32ColorBaronyBlue(*mainsurface), language[2621]);
						}
						else if ( timeDiff == 350 )
						{
							playSound(348, orangeSpeechVolume);
							messagePlayerColor(clientnum, uint32ColorOrange(*mainsurface), language[2622]);
							playerLevelEntrySpeech = 0;
						}
						break;
					default:
						break;
				}
				break;
			case 28:
				switch ( playerLevelEntrySpeech )
				{
					case 1:
						if ( timeDiff == 200 )
						{
							playSound(350, orangeSpeechVolume);
							messagePlayerColor(clientnum, uint32ColorOrange(*mainsurface), language[2630]);
						}
						else if ( timeDiff == 350 )
						{
							playSound(351, blueSpeechVolume);
							messagePlayerColor(clientnum, uint32ColorBaronyBlue(*mainsurface), language[2631]);
							playerLevelEntrySpeech = 0;
						}
						break;
					case 2:
						if ( timeDiff == 350 )
						{
							playSound(353, blueSpeechVolume);
							messagePlayerColor(clientnum, uint32ColorBaronyBlue(*mainsurface), language[2633]);
							playerLevelEntrySpeech = 0;
						}
						break;
					case 3:
						if ( timeDiff == 200 )
						{
							playSound(355, orangeSpeechVolume);
							messagePlayerColor(clientnum, uint32ColorOrange(*mainsurface), language[2635]);
							playerLevelEntrySpeech = 0;
						}
						break;
					default:
						break;
				}
				break;
			case 30:
				switch ( playerLevelEntrySpeech )
				{
					case 1:
						if ( timeDiff == 350 )
						{
							playSound(357, orangeSpeechVolume);
							messagePlayerColor(clientnum, uint32ColorOrange(*mainsurface), language[2637]);
						}
						else if ( timeDiff == 500 )
						{
							messagePlayerColor(clientnum, uint32ColorOrange(*mainsurface), language[2652]);
							playerLevelEntrySpeech = 0;
						}
						break;
					default:
						break;
				}
				break;
			case 31:
				switch ( playerLevelEntrySpeech )
				{
					case 1:
						if ( timeDiff == 350 )
						{
							playSound(359, orangeSpeechVolume);
							messagePlayerColor(clientnum, uint32ColorOrange(*mainsurface), language[2639]);
						}
						else if ( timeDiff == 510 )
						{
							messagePlayerColor(clientnum, uint32ColorOrange(*mainsurface), language[2653]);
							playerLevelEntrySpeech = 0;
						}
						break;
					default:
						break;
				}
				break;
			case 33:
				switch ( playerLevelEntrySpeech )
				{
					case 1:
						if ( timeDiff == 200 )
						{
							playSound(361, blueSpeechVolume);
							messagePlayerColor(clientnum, uint32ColorBaronyBlue(*mainsurface), language[2641]);
							playerLevelEntrySpeech = 0;
						}
						break;
					case 2:
						if ( timeDiff == 350 )
						{
							playSound(363, orangeSpeechVolume);
							messagePlayerColor(clientnum, uint32ColorOrange(*mainsurface), language[2643]);
							playerLevelEntrySpeech = 0;
						}
						break;
					default:
						break;
				}
				break;
			case 35:
				switch ( playerLevelEntrySpeech )
				{
					case 1:
						if ( timeDiff == 310 )
						{
							playSound(365, blueSpeechVolume);
							messagePlayerColor(clientnum, uint32ColorBaronyBlue(*mainsurface), language[2645]);
							playerLevelEntrySpeech = 0;
						}
						break;
					default:
						break;
				}
				break;
			case 27:
			case 29:
			case 32:
			case 34:
				if ( minotaurlevel )
				{
					switch ( playerLevelEntrySpeech )
					{
						case 1:
							if ( timeDiff == 200 )
							{
								playSound(367, orangeSpeechVolume);
								messagePlayerColor(clientnum, uint32ColorOrange(*mainsurface), language[2624]);
								playerLevelEntrySpeech = 0;
							}
							break;
						case 2:
							if ( timeDiff == 200 )
							{
								playSound(369, blueSpeechVolume);
								messagePlayerColor(clientnum, uint32ColorBaronyBlue(*mainsurface), language[2626]);
								playerLevelEntrySpeech = 0;
							}
							break;
						case 3:
							if ( timeDiff == 200 )
							{
								playSound(371, orangeSpeechVolume);
								messagePlayerColor(clientnum, uint32ColorOrange(*mainsurface), language[2628]);
								playerLevelEntrySpeech = 0;
							}
							break;
						default:
							break;
					}
					break;
				}
				break;
			default:
				break;
		}

	}
}

bool Entity::isPlayerHeadSprite()
{
	switch ( sprite )
	{
		case 113:
		case 114:
		case 115:
		case 116:
		case 117:
		case 125:
		case 126:
		case 127:
		case 128:
		case 129:
		case 332:
		case 333:
		case 341:
		case 342:
		case 343:
		case 344:
		case 345:
		case 346:
		case 354:
		case 355:
		case 356:
		case 357:
		case 358:
		case 359:
		case 367:
		case 368:
		case 369:
		case 370:
		case 371:
		case 372:
		case 380:
		case 381:
		case 382:
		case 383:
		case 384:
		case 385:
		case 686:
		case 694:
		case 702:
		case 710:
		case 718:
		case 726:
		case 734:
		case 742:
		case 752:
		case 756:
		case 760:
		case 768:
		case 770:
			return true;
			break;
		default:
			break;
	}
	return false;
}

Monster Entity::getMonsterFromPlayerRace(int playerRace)
{
	switch ( playerRace )
	{
		case RACE_HUMAN:
			return HUMAN;
			break;
		case RACE_SKELETON:
			return SKELETON;
			break;
		case RACE_INCUBUS:
			return INCUBUS;
			/*if ( stats[this->skill[2]]->sex == FEMALE )
			{
				return SUCCUBUS;
			}
			else
			{
			}*/
			break;
		case RACE_GOBLIN:
			return GOBLIN;
			break;
		case RACE_AUTOMATON:
			return AUTOMATON;
			break;
		case RACE_INSECTOID:
			return INSECTOID;
			break;
		case RACE_GOATMAN:
			return GOATMAN;
			break;
		case RACE_VAMPIRE:
			return VAMPIRE;
			break;
		case RACE_SUCCUBUS:
			return SUCCUBUS;
			break;
		default:
			return HUMAN;
			break;
	}
	return HUMAN;
}

void Entity::setDefaultPlayerModel(int playernum, Monster playerRace, int limbType)
{
	if ( !players[playernum] || !players[playernum]->entity )
	{
		return;
	}

	int playerAppearance = stats[playernum]->appearance;
	if ( players[playernum] && players[playernum]->entity && players[playernum]->entity->effectPolymorph > NUMMONSTERS )
	{
		playerAppearance = players[playernum]->entity->effectPolymorph - 100;
	}

	if ( limbType == LIMB_HUMANOID_TORSO )
	{
		if ( playerRace == HUMAN )
		{
			switch ( playerAppearance / 6 )
			{
				case 1:
					this->sprite = 334 + 13 * stats[playernum]->sex;
					break;
				case 2:
					this->sprite = 360 + 13 * stats[playernum]->sex;
					break;
				default:
					this->sprite = 106 + 12 * stats[playernum]->sex;
					break;
			}
		}
		else
		{
			switch ( playerRace )
			{
				case SKELETON:
					this->sprite = 687;
					break;
				case GOBLIN:
					if ( stats[playernum]->sex == FEMALE )
					{
						this->sprite = 753;
					}
					else
					{
						this->sprite = 695;
					}
					break;
				case INCUBUS:
					this->sprite = 703;
					break;
				case SUCCUBUS:
					this->sprite = 711;
					break;
				case VAMPIRE:
					if ( stats[playernum]->sex == FEMALE )
					{
						this->sprite = 757;
					}
					else
					{
						this->sprite = 719;
					}
					break;
				case INSECTOID:
					if ( stats[playernum]->sex == FEMALE )
					{
						this->sprite = 761;
					}
					else
					{
						this->sprite = 727;
					}
					break;
				case GOATMAN:
					if ( stats[playernum]->sex == FEMALE )
					{
						this->sprite = 769;
					}
					else
					{
						this->sprite = 735;
					}
					break;
				case AUTOMATON:
					this->sprite = 743;
					break;
				default:
					break;
			}
		}
	}
	else if ( limbType == LIMB_HUMANOID_RIGHTLEG )
	{
		if ( playerRace == HUMAN )
		{
			switch ( playerAppearance / 6 )
			{
				case 1:
					this->sprite = 335 + 13 * stats[playernum]->sex;
					break;
				case 2:
					this->sprite = 361 + 13 * stats[playernum]->sex;
					break;
				default:
					this->sprite = 107 + 12 * stats[playernum]->sex;
					break;
			}
		}
		else
		{
			switch ( playerRace )
			{
				case SKELETON:
					this->sprite = 693;
					break;
				case GOBLIN:
					if ( stats[playernum]->sex == FEMALE )
					{
						this->sprite = 755;
					}
					else
					{
						this->sprite = 701;
					}
					break;
				case INCUBUS:
					this->sprite = 709;
					break;
				case SUCCUBUS:
					this->sprite = 717;
					break;
				case VAMPIRE:
					if ( stats[playernum]->sex == FEMALE )
					{
						this->sprite = 759;
					}
					else
					{
						this->sprite = 725;
					}
					break;
				case INSECTOID:
					if ( stats[playernum]->sex == FEMALE )
					{
						this->sprite = 767;
					}
					else
					{
						this->sprite = 733;
					}
					break;
				case GOATMAN:
					this->sprite = 741;
					break;
				case AUTOMATON:
					this->sprite = 749;
					break;
				default:
					break;
			}
		}
	}
	else if ( limbType == LIMB_HUMANOID_LEFTLEG )
	{
		if ( playerRace == HUMAN )
		{
			switch ( playerAppearance / 6 )
			{
				case 1:
					this->sprite = 336 + 13 * stats[playernum]->sex;
					break;
				case 2:
					this->sprite = 362 + 13 * stats[playernum]->sex;
					break;
				default:
					this->sprite = 108 + 12 * stats[playernum]->sex;
					break;
			}
		}
		else
		{
			switch ( playerRace )
			{
				case SKELETON:
					this->sprite = 692;
					break;
				case GOBLIN:
					if ( stats[playernum]->sex == FEMALE )
					{
						this->sprite = 754;
					}
					else
					{
						this->sprite = 700;
					}
					break;
				case INCUBUS:
					this->sprite = 708;
					break;
				case SUCCUBUS:
					this->sprite = 716;
					break;
				case VAMPIRE:
					if ( stats[playernum]->sex == FEMALE )
					{
						this->sprite = 758;
					}
					else
					{
						this->sprite = 724;
					}
					break;
				case INSECTOID:
					if ( stats[playernum]->sex == FEMALE )
					{
						this->sprite = 766;
					}
					else
					{
						this->sprite = 732;
					}
					break;
				case GOATMAN:
					this->sprite = 740;
					break;
				case AUTOMATON:
					this->sprite = 748;
					break;
				default:
					break;
			}
		}
	}
	else if ( limbType == LIMB_HUMANOID_RIGHTARM )
	{
		if ( playerRace == HUMAN )
		{
			switch ( playerAppearance / 6 )
			{
				case 1:
					this->sprite = 337 + 13 * stats[playernum]->sex;
					break;
				case 2:
					this->sprite = 363 + 13 * stats[playernum]->sex;
					break;
				default:
					this->sprite = 109 + 12 * stats[playernum]->sex;
					break;
			}
		}
		else
		{
			switch ( playerRace )
			{
				case SKELETON:
					this->sprite = 689;
					break;
				case GOBLIN:
					this->sprite = 697;
					break;
				case INCUBUS:
					this->sprite = 705;
					break;
				case SUCCUBUS:
					this->sprite = 713;
					break;
				case VAMPIRE:
					this->sprite = 721;
					break;
				case INSECTOID:
					if ( stats[playernum]->sex == FEMALE )
					{
						this->sprite = 763;
					}
					else
					{
						this->sprite = 729;
					}
					break;
				case GOATMAN:
					this->sprite = 737;
					break;
				case AUTOMATON:
					this->sprite = 745;
					break;
				default:
					break;
			}
		}
	}
	else if ( limbType == LIMB_HUMANOID_LEFTARM )
	{
		if ( playerRace == HUMAN )
		{
			switch ( playerAppearance / 6 )
			{
				case 1:
					this->sprite = 338 + 13 * stats[playernum]->sex;
					break;
				case 2:
					this->sprite = 364 + 13 * stats[playernum]->sex;
					break;
				default:
					this->sprite = 110 + 12 * stats[playernum]->sex;
					break;
			}
		}
		else
		{
			switch ( playerRace )
			{
				case SKELETON:
					this->sprite = 688;
					break;
				case GOBLIN:
					this->sprite = 696;
					break;
				case INCUBUS:
					this->sprite = 704;
					break;
				case SUCCUBUS:
					this->sprite = 712;
					break;
				case VAMPIRE:
					this->sprite = 720;
					break;
				case INSECTOID:
					if ( stats[playernum]->sex == FEMALE )
					{
						this->sprite = 762;
					}
					else
					{
						this->sprite = 728;
					}
					break;
				case GOATMAN:
					this->sprite = 736;
					break;
				case AUTOMATON:
					this->sprite = 744;
					break;
				default:
					break;
			}
		}
	}
}

bool Entity::playerRequiresBloodToSustain()
{
	if ( behavior != &actPlayer )
	{
		return false;
	}
	if ( !stats[skill[2]] )
	{
		return false;
	}

	if ( stats[skill[2]]->type == VAMPIRE )
	{
		return true;
	}
	if ( stats[skill[2]]->EFFECTS[EFF_VAMPIRICAURA] || client_classes[skill[2]] == CLASS_ACCURSED )
	{
		return true;
	}
	if ( stats[skill[2]]->playerRace == VAMPIRE && stats[skill[2]]->appearance == 0 )
	{
		return true;
	}
	
	return false;
}