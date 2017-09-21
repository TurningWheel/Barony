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

bool smoothmouse = false;
bool settings_smoothmouse = false;

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

	if (my->light)
	{
		list_RemoveNode(my->light->node);
		my->light = nullptr;
	}
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
#define PLAYERWALKSPEED .12

void actPlayer(Entity* my)
{
	if (!my)
	{
		return;
	}

	Entity* entity;
	Entity* entity2 = NULL;
	Entity* rightbody = NULL;
	Entity* weaponarm = NULL;
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

		// hud weapon
		if ( PLAYER_NUM == clientnum )
		{
			if ( multiplayer == CLIENT )
			{
				my->flags[UPDATENEEDED] = false;
			}

			entity = newEntity(-1, 1, map.entities);
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

			// magic hands

			//Left hand.
			entity = newEntity(-1, 1, map.entities);
			entity->flags[PASSABLE] = true;
			entity->flags[OVERDRAW] = true;
			entity->flags[NOUPDATE] = true;
			entity->flags[INVISIBLE] = true;
			entity->behavior = &actLeftHandMagic;
			entity->focalz = -4;
			magicLeftHand = entity;
			//Right hand.
			entity = newEntity(-1, 1, map.entities);
			entity->flags[PASSABLE] = true;
			entity->flags[OVERDRAW] = true;
			entity->flags[NOUPDATE] = true;
			entity->flags[INVISIBLE] = true;
			entity->behavior = &actRightHandMagic;
			entity->focalz = -4;
			magicRightHand = entity;

			// hud shield
			entity = newEntity(-1, 1, map.entities);
			entity->flags[PASSABLE] = true;
			entity->flags[OVERDRAW] = true;
			entity->flags[NOUPDATE] = true;
			entity->flags[INVISIBLE] = true;
			entity->behavior = &actHudShield;
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
		entity = newEntity(106 + 12 * stats[PLAYER_NUM]->sex, 1, map.entities);
		entity->sizex = 4;
		entity->sizey = 4;
		entity->skill[2] = PLAYER_NUM;
		entity->flags[PASSABLE] = true;
		entity->flags[NOUPDATE] = true;
		entity->flags[GENIUS] = true;
		entity->focalx = limbs[HUMAN][1][0];
		entity->focaly = limbs[HUMAN][1][1];
		entity->focalz = limbs[HUMAN][1][2];
		entity->behavior = &actPlayerLimb;
		entity->parent = my->getUID();
		node = list_AddNodeLast(&my->children);
		node->element = entity;
		node->deconstructor = &emptyDeconstructor;
		node->size = sizeof(Entity*);

		// right leg
		entity = newEntity(107 + 12 * stats[PLAYER_NUM]->sex, 1, map.entities);
		entity->sizex = 4;
		entity->sizey = 4;
		entity->skill[2] = PLAYER_NUM;
		entity->flags[PASSABLE] = true;
		entity->flags[NOUPDATE] = true;
		entity->flags[GENIUS] = true;
		entity->focalx = limbs[HUMAN][2][0];
		entity->focaly = limbs[HUMAN][2][1];
		entity->focalz = limbs[HUMAN][2][2];
		entity->behavior = &actPlayerLimb;
		entity->parent = my->getUID();
		node = list_AddNodeLast(&my->children);
		node->element = entity;
		node->deconstructor = &emptyDeconstructor;
		node->size = sizeof(Entity*);

		// left leg
		entity = newEntity(108 + 12 * stats[PLAYER_NUM]->sex, 1, map.entities);
		entity->sizex = 4;
		entity->sizey = 4;
		entity->skill[2] = PLAYER_NUM;
		entity->flags[PASSABLE] = true;
		entity->flags[NOUPDATE] = true;
		entity->flags[GENIUS] = true;
		entity->focalx = limbs[HUMAN][3][0];
		entity->focaly = limbs[HUMAN][3][1];
		entity->focalz = limbs[HUMAN][3][2];
		entity->behavior = &actPlayerLimb;
		entity->parent = my->getUID();
		node = list_AddNodeLast(&my->children);
		node->element = entity;
		node->deconstructor = &emptyDeconstructor;
		node->size = sizeof(Entity*);

		// right arm
		entity = newEntity(109 + 12 * stats[PLAYER_NUM]->sex, 1, map.entities);
		entity->sizex = 4;
		entity->sizey = 4;
		entity->skill[2] = PLAYER_NUM;
		entity->flags[PASSABLE] = true;
		entity->flags[NOUPDATE] = true;
		entity->flags[GENIUS] = true;
		entity->focalx = limbs[HUMAN][4][0];
		entity->focaly = limbs[HUMAN][4][1];
		entity->focalz = limbs[HUMAN][4][2];
		entity->behavior = &actPlayerLimb;
		entity->parent = my->getUID();
		node = list_AddNodeLast(&my->children);
		node->element = entity;
		node->deconstructor = &emptyDeconstructor;
		node->size = sizeof(Entity*);

		// left arm
		entity = newEntity(110 + 12 * stats[PLAYER_NUM]->sex, 1, map.entities);
		entity->sizex = 4;
		entity->sizey = 4;
		entity->skill[2] = PLAYER_NUM;
		entity->flags[PASSABLE] = true;
		entity->flags[NOUPDATE] = true;
		entity->flags[GENIUS] = true;
		entity->focalx = limbs[HUMAN][5][0];
		entity->focaly = limbs[HUMAN][5][1];
		entity->focalz = limbs[HUMAN][5][2];
		entity->behavior = &actPlayerLimb;
		entity->parent = my->getUID();
		node = list_AddNodeLast(&my->children);
		node->element = entity;
		node->deconstructor = &emptyDeconstructor;
		node->size = sizeof(Entity*);

		// world weapon
		entity = newEntity(-1, 1, map.entities);
		entity->sizex = 4;
		entity->sizey = 4;
		entity->skill[2] = PLAYER_NUM;
		entity->flags[PASSABLE] = true;
		entity->flags[NOUPDATE] = true;
		entity->flags[GENIUS] = true;
		entity->flags[INVISIBLE] = true;
		entity->focalx = limbs[HUMAN][6][0];
		entity->focaly = limbs[HUMAN][6][1];
		entity->focalz = limbs[HUMAN][6][2];
		entity->behavior = &actPlayerLimb;
		entity->parent = my->getUID();
		node = list_AddNodeLast(&my->children);
		node->element = entity;
		node->deconstructor = &emptyDeconstructor;
		node->size = sizeof(Entity*);

		// shield
		entity = newEntity(-1, 1, map.entities);
		entity->sizex = 4;
		entity->sizey = 4;
		entity->skill[2] = PLAYER_NUM;
		entity->flags[PASSABLE] = true;
		entity->flags[NOUPDATE] = true;
		entity->flags[GENIUS] = true;
		entity->flags[INVISIBLE] = true;
		entity->focalx = limbs[HUMAN][7][0];
		entity->focaly = limbs[HUMAN][7][1];
		entity->focalz = limbs[HUMAN][7][2];
		entity->behavior = &actPlayerLimb;
		entity->parent = my->getUID();
		entity->focalx = 2;
		node = list_AddNodeLast(&my->children);
		node->element = entity;
		node->deconstructor = &emptyDeconstructor;
		node->size = sizeof(Entity*);

		// cloak
		entity = newEntity(-1, 1, map.entities);
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
		entity->focalx = limbs[HUMAN][8][0];
		entity->focaly = limbs[HUMAN][8][1];
		entity->focalz = limbs[HUMAN][8][2];
		entity->behavior = &actPlayerLimb;
		entity->parent = my->getUID();
		node = list_AddNodeLast(&my->children);
		node->element = entity;
		node->deconstructor = &emptyDeconstructor;
		node->size = sizeof(Entity*);

		// helmet
		entity = newEntity(-1, 1, map.entities);
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
		entity->focalx = limbs[HUMAN][9][0];
		entity->focaly = limbs[HUMAN][9][1];
		entity->focalz = limbs[HUMAN][9][2];
		entity->behavior = &actPlayerLimb;
		entity->parent = my->getUID();
		node = list_AddNodeLast(&my->children);
		node->element = entity;
		node->deconstructor = &emptyDeconstructor;
		node->size = sizeof(Entity*);

		// mask
		entity = newEntity(-1, 1, map.entities);
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
		entity->focalx = limbs[HUMAN][10][0];
		entity->focaly = limbs[HUMAN][10][1];
		entity->focalz = limbs[HUMAN][10][2];
		entity->behavior = &actPlayerLimb;
		entity->parent = my->getUID();
		node = list_AddNodeLast(&my->children);
		node->element = entity;
		node->deconstructor = &emptyDeconstructor;
		node->size = sizeof(Entity*);
	}
	Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 255);

	if ( !intro )
	{
		PLAYER_ALIVETIME++;
		if ( PLAYER_NUM == clientnum )
		{
			clientplayer = my->getUID();
			if ( !strcmp(map.name, "Boss") && !my->skill[29] )
			{
				bool foundherx = false;
				for ( node = map.entities->first; node != NULL; node = node->next )
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
					else if ( minotaurlevel )
					{
						int speech = rand() % 3;
						playSound(123 + speech, 128);
						messagePlayerColor(clientnum, color, language[537]);
						messagePlayerColor(clientnum, color, language[74 + speech]);
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
					else if ( minotaurlevel )
					{
						int speech = rand() % 3;
						playSound(129 + speech, 128);
						messagePlayerColor(clientnum, color, language[80 + speech]);
					}
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
	}

    if ( PLAYER_NUM == clientnum )
    {
        if ( appraisalGUI->IsGUIOpen() == true )
        {
            appraisalGUI->UpdateGUI();
        }
    }

    /*
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
					}
					else
					{
						messagePlayer(clientnum, language[571], tempItem->description());
					}

					//Attempt a level up.
					if ( items[tempItem->type].value > 0 )
					{
						if ( tempItem->identified )
						{
							my->increaseSkill(PRO_APPRAISAL);
						}
						else if ( rand() % 5 == 0 )
						{
							my->increaseSkill(PRO_APPRAISAL);
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
	}*/

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
			my->z = 1.5;
			my->pitch = PI / 4;
		}
		else if ( !noclip )
		{
			my->z = -1;
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
						messagePlayer(PLAYER_NUM, language[572]);
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
			if ( animatedtiles[map.tiles[y * MAPLAYERS + x * MAPLAYERS * map.height]] )
			{
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
					else
					{
						playSound(136, 128);
					}
				}
				if ( multiplayer != CLIENT )
				{
					if ( !lavatiles[map.tiles[y * MAPLAYERS + x * MAPLAYERS * map.height]] )
					{
						if ( my->flags[BURNING] )
						{
							my->flags[BURNING] = false;
							messagePlayer(PLAYER_NUM, language[574]);
							if ( PLAYER_NUM > 0 )
							{
								serverUpdateEntityFlag(my, BURNING);
							}
						}
					}
					else if ( ticks % 10 == 0 )
					{
						my->modHP(-2 - rand() % 2);
						my->setObituary(language[1506]);
						if ( !my->flags[BURNING] )
						{
							my->flags[BURNING] = true;
							if ( PLAYER_NUM > 0 )
							{
								serverUpdateEntityFlag(my, BURNING);
							}
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
			if ( (*inputPressed(impulses[IN_FORWARD]) || *inputPressed(impulses[IN_BACK])) || (*inputPressed(impulses[IN_RIGHT]) - *inputPressed(impulses[IN_LEFT])) || (game_controller && (game_controller->getLeftXPercent() || game_controller->getLeftYPercent())) && !command && !swimming)
			{
				if (!stats[clientnum]->defending)
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
			if ( !swimming && !stats[clientnum]->defending )
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
			selectedEntity = entityClicked(); // using objects

			if ( selectedEntity != NULL )
			{
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
						PLAYER_TORCH = 7 + my->getPER() / 3;
					}
					else if ( stats[PLAYER_NUM]->shield->type == TOOL_LANTERN )
					{
						PLAYER_TORCH = 10 + my->getPER() / 3;
					}
					else if ( stats[PLAYER_NUM]->shield->type == TOOL_CRYSTALSHARD )
					{
						PLAYER_TORCH = 5 + my->getPER() / 3;
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
					PLAYER_TORCH = 3 + my->getPER() / 3;
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
	if ( my->light != NULL )
	{
		list_RemoveNode(my->light->node);
		my->light = NULL;
	}
	if ( PLAYER_TORCH && my->light == NULL )
	{
		my->light = lightSphereShadow(my->x / 16, my->y / 16, PLAYER_TORCH, 50 + 15 * PLAYER_TORCH);
	}

	// server controls players primarily
	if ( PLAYER_NUM == clientnum || multiplayer == SERVER )
	{
		// set head model
		if ( stats[PLAYER_NUM]->appearance < 5 )
		{
			my->sprite = 113 + 12 * stats[PLAYER_NUM]->sex + stats[PLAYER_NUM]->appearance;
		}
		else if ( stats[PLAYER_NUM]->appearance == 5 )
		{
			my->sprite = 332 + stats[PLAYER_NUM]->sex;
		}
		else if ( stats[PLAYER_NUM]->appearance >= 6 && stats[PLAYER_NUM]->appearance < 12 )
		{
			my->sprite = 341 + stats[PLAYER_NUM]->sex * 13 + stats[PLAYER_NUM]->appearance - 6;
		}
		else if ( stats[PLAYER_NUM]->appearance >= 12 )
		{
			my->sprite = 367 + stats[PLAYER_NUM]->sex * 13 + stats[PLAYER_NUM]->appearance - 12;
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
						list_RemoveNode(tempEntity->mynode);
						if ( i > 10 )
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
								entity = newEntity(160, 1, map.entities);
								entity->x = my->x;
								entity->y = my->y;
								entity->z = 7.4 + (rand() % 20) / 100.f;
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
						snprintf(whatever, 255, "%s %s", stats[PLAYER_NUM]->name, stats[PLAYER_NUM]->obituary);
						messagePlayer(c, whatever);
					}
					Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
					messagePlayerColor(PLAYER_NUM, color, language[577]);
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
						entity = newEntity(-1, 1, map.entities);
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

						deleteSaveGame(); // stops save scumming c:

						closeBookGUI();

#ifdef SOUND
						levelmusicplaying = true;
						combatmusicplaying = false;
						fadein_increment = default_fadein_increment * 4;
						fadeout_increment = default_fadeout_increment * 4;
						playmusic(sounds[209], false, true, false);
#endif
						combat = false;
						for (node = stats[PLAYER_NUM]->inventory.first; node != nullptr; node = nextnode)
						{
							nextnode = node->next;
							Item* item = (Item*)node->element;
							if (itemCategory(item) == SPELL_CAT)
							{
								continue;    // don't drop spells on death, stupid!
							}
							int c = item->count;
							for (c = item->count; c > 0; c--)
							{
								entity = newEntity(-1, 1, map.entities);
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

					if ( multiplayer != SINGLE )
					{
						messagePlayer(PLAYER_NUM, language[578]);
					}
				}
				if ( my->light != NULL )
				{
					list_RemoveNode(my->light->node);
					my->light = NULL;
				}
				list_RemoveNode(my->mynode);
				return;
			}
		}
	}

	if ( PLAYER_NUM == clientnum && intro == false )
	{
		// effects of drunkenness
		if ( stats[PLAYER_NUM]->EFFECTS[EFF_DRUNK] == true )
		{
			CHAR_DRUNK++;
			if ( CHAR_DRUNK >= 180 )
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
				if ( item->type >= 0 && item->type < NUMITEMS )
				{
					weight += items[item->type].weight * item->count;
				}
		}
		weight += stats[PLAYER_NUM]->GOLD / 100;
		weightratio = (1000 + my->getSTR() * 100 - weight) / (double)(1000 + my->getSTR() * 100);
		weightratio = fmin(fmax(0, weightratio), 1);

		// calculate movement forces
		if ( !command )
		{
			//x_force and y_force represent the amount of percentage pushed on that respective axis. Given a keyboard, it's binary; either you're pushing "move left" or you aren't. On an analog stick, it can range from whatever value to whatever.
			float x_force = 0;
			float y_force = 0;
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

			PLAYER_VELX += y_force * cos(my->yaw) * .045 * (my->getDEX() + 10) * weightratio / (1 + stats[PLAYER_NUM]->defending);
			PLAYER_VELY += y_force * sin(my->yaw) * .045 * (my->getDEX() + 10) * weightratio / (1 + stats[PLAYER_NUM]->defending);
			PLAYER_VELX += x_force * cos(my->yaw + PI / 2) * .0225 * (my->getDEX() + 10) * weightratio / (1 + stats[PLAYER_NUM]->defending);
			PLAYER_VELY += x_force * sin(my->yaw + PI / 2) * .0225 * (my->getDEX() + 10) * weightratio / (1 + stats[PLAYER_NUM]->defending);
		}
		PLAYER_VELX *= .75;
		PLAYER_VELY *= .75;

		for ( node = map.entities->first; node != NULL; node = node->next )
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
			if ( stats[PLAYER_NUM]->amulet->type == AMULET_WATERBREATHING )
			{
				amuletwaterbreathing = true;
			}
		if ( swimming && !amuletwaterbreathing )
		{
			PLAYER_VELX *= (((stats[PLAYER_NUM]->PROFICIENCIES[PRO_SWIMMING] / 100.f) * 50.f) + 50) / 100.f;
			PLAYER_VELY *= (((stats[PLAYER_NUM]->PROFICIENCIES[PRO_SWIMMING] / 100.f) * 50.f) + 50) / 100.f;
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
			net_packet->address.host = net_server.host;
			net_packet->address.port = net_server.port;
			net_packet->len = 18;
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
						if ( hit.entity->skill[0] == 0 || (hit.entity->skill[0] == 3 && hit.entity->skill[1] == 0) )
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
						if ( hit.entity->skill[0] == 0 || (hit.entity->skill[0] == 3 && hit.entity->skill[1] == 0) )
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
		entity->yaw = my->yaw;
		if ( bodypart == 2 || bodypart == 5 )
		{
			if ( bodypart == 2 )
			{
				rightbody = (Entity*)node->next->element;
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
		else if ( bodypart == 3 || bodypart == 4 || bodypart == 8 )
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
						entity->pitch = -3 * PI / 4;
						entity->roll = 0;
					}
					else
					{
						if ( entity->pitch >= -PI / 2 )
						{
							PLAYER_ARMBENDED = 1;
						}
						if ( entity->pitch >= PI / 4 )
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
							entity->pitch += .25;
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
						entity->pitch = 2 * PI / 3;
						entity->roll = 0;
					}
					else
					{
						if ( PLAYER_ATTACKTIME >= 5 )
						{
							PLAYER_ARMBENDED = 1;
							entity->pitch = -PI / 6;
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
				if ( multiplayer != CLIENT )
				{
					if ( stats[PLAYER_NUM]->breastplate == NULL )
					{
						switch ( stats[PLAYER_NUM]->appearance / 6 )
						{
							case 1:
								entity->sprite = 334 + 13 * stats[PLAYER_NUM]->sex;
								break;
							case 2:
								entity->sprite = 360 + 13 * stats[PLAYER_NUM]->sex;
								break;
							default:
								entity->sprite = 106 + 12 * stats[PLAYER_NUM]->sex;
								break;
						}
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
				entity->x -= .25 * cos(my->yaw);
				entity->y -= .25 * sin(my->yaw);
				entity->z += 2.5;
				break;
			// right leg
			case 2:
				if ( multiplayer != CLIENT )
				{
					if ( stats[PLAYER_NUM]->shoes == NULL )
					{
						switch ( stats[PLAYER_NUM]->appearance / 6 )
						{
							case 1:
								entity->sprite = 335 + 13 * stats[PLAYER_NUM]->sex;
								break;
							case 2:
								entity->sprite = 361 + 13 * stats[PLAYER_NUM]->sex;
								break;
							default:
								entity->sprite = 107 + 12 * stats[PLAYER_NUM]->sex;
								break;
						}
					}
					else
					{
						if ( setBootSprite(stats[PLAYER_NUM], entity, SPRITE_BOOT_RIGHT_OFFSET) != 0 )
						{
							// successfully set sprite for the human model
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
						if ( PLAYER_ALIVETIME == TICKS_PER_SECOND + bodypart )
						{
							serverUpdateEntityBodypart(my, bodypart);
						}
					}
				}
				entity->x += 1 * cos(my->yaw + PI / 2) + .25 * cos(my->yaw);
				entity->y += 1 * sin(my->yaw + PI / 2) + .25 * sin(my->yaw);
				entity->z += 5;
				if ( my->z >= 1.4 && my->z <= 1.6 )
				{
					entity->yaw += PI / 8;
					entity->pitch = -PI / 2;
				}
				break;
			// left leg
			case 3:
				if ( multiplayer != CLIENT )
				{
					if ( stats[PLAYER_NUM]->shoes == NULL )
					{
						switch ( stats[PLAYER_NUM]->appearance / 6 )
						{
							case 1:
								entity->sprite = 336 + 13 * stats[PLAYER_NUM]->sex;
								break;
							case 2:
								entity->sprite = 362 + 13 * stats[PLAYER_NUM]->sex;
								break;
							default:
								entity->sprite = 108 + 12 * stats[PLAYER_NUM]->sex;
								break;
						}
					}
					else
					{
						if ( setBootSprite(stats[PLAYER_NUM], entity, SPRITE_BOOT_LEFT_OFFSET) != 0 )
						{
							// successfully set sprite for the human model
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
						if ( PLAYER_ALIVETIME == TICKS_PER_SECOND + bodypart )
						{
							serverUpdateEntityBodypart(my, bodypart);
						}
					}
				}
				entity->x -= 1 * cos(my->yaw + PI / 2) - .25 * cos(my->yaw);
				entity->y -= 1 * sin(my->yaw + PI / 2) - .25 * sin(my->yaw);
				entity->z += 5;
				if ( my->z >= 1.4 && my->z <= 1.6 )
				{
					entity->yaw -= PI / 8;
					entity->pitch = -PI / 2;
				}
				break;
			// right arm
			case 4:
			{
				if ( multiplayer != CLIENT )
				{
					if ( stats[PLAYER_NUM]->gloves == NULL )
					{
						switch ( stats[PLAYER_NUM]->appearance / 6 )
						{
							case 1:
								entity->sprite = 337 + 13 * stats[PLAYER_NUM]->sex;
								break;
							case 2:
								entity->sprite = 363 + 13 * stats[PLAYER_NUM]->sex;
								break;
							default:
								entity->sprite = 109 + 12 * stats[PLAYER_NUM]->sex;
								break;
						}
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
				entity->x += 2.5 * cos(my->yaw + PI / 2) - .20 * cos(my->yaw);
				entity->y += 2.5 * sin(my->yaw + PI / 2) - .20 * sin(my->yaw);
				entity->z += 1.5;
				node_t* tempNode = list_Node(&my->children, 6);
				if ( tempNode )
				{
					Entity* weapon = (Entity*)tempNode->element;
					/*if( multiplayer==CLIENT ) {
						if( !PLAYER_ARMBENDED ) {
							if( entity->skill[7]==0 )
								entity->skill[7] = entity->sprite;
							entity->sprite = entity->skill[7];
							entity->sprite += 2*(weapon->flags[INVISIBLE]!=true);
						}
					}*/
					if ( weapon->flags[INVISIBLE] || PLAYER_ARMBENDED )
					{
						entity->focalx = limbs[HUMAN][4][0]; // 0
						entity->focaly = limbs[HUMAN][4][1]; // 0
						entity->focalz = limbs[HUMAN][4][2]; // 1.5
					}
					else
					{
						entity->focalx = limbs[HUMAN][4][0] + 0.75;
						entity->focaly = limbs[HUMAN][4][1];
						entity->focalz = limbs[HUMAN][4][2] - 0.75;
					}
				}
				entity->yaw += PLAYER_WEAPONYAW;
				if ( my->z >= 1.4 && my->z <= 1.6 )
				{
					entity->pitch = 0;
				}
				break;
			}
			// left arm
			case 5:
			{
				if ( multiplayer != CLIENT )
				{
					if ( stats[PLAYER_NUM]->gloves == NULL )
					{
						switch ( stats[PLAYER_NUM]->appearance / 6 )
						{
							case 1:
								entity->sprite = 338 + 13 * stats[PLAYER_NUM]->sex;
								break;
							case 2:
								entity->sprite = 364 + 13 * stats[PLAYER_NUM]->sex;
								break;
							default:
								entity->sprite = 110 + 12 * stats[PLAYER_NUM]->sex;
								break;
						}
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
				entity->x -= 2.5 * cos(my->yaw + PI / 2) + .20 * cos(my->yaw);
				entity->y -= 2.5 * sin(my->yaw + PI / 2) + .20 * sin(my->yaw);
				entity->z += 1.5;
				node_t* tempNode = list_Node(&my->children, 7);
				if ( tempNode )
				{
					Entity* shield = (Entity*)tempNode->element;
					if ( shield->flags[INVISIBLE] )
					{
						entity->focalx = limbs[HUMAN][5][0]; // 0
						entity->focaly = limbs[HUMAN][5][1]; // 0
						entity->focalz = limbs[HUMAN][5][2]; // 1.5
					}
					else
					{
						entity->focalx = limbs[HUMAN][5][0] + 0.75;
						entity->focaly = limbs[HUMAN][5][1];
						entity->focalz = limbs[HUMAN][5][2] - 0.75;
					}
				}
				if ( my->z >= 1.4 && my->z <= 1.6 )
				{
					entity->pitch = 0;
				}
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
				if ( weaponarm != NULL )
				{
					if ( entity->sprite == items[SHORTBOW].index )
					{
						entity->x = weaponarm->x - .5 * cos(weaponarm->yaw);
						entity->y = weaponarm->y - .5 * sin(weaponarm->yaw);
						entity->z = weaponarm->z + 1;
						entity->pitch = weaponarm->pitch + .25;
					}
					else if ( entity->sprite == items[CROSSBOW].index )
					{
						entity->x = weaponarm->x;
						entity->y = weaponarm->y;
						entity->z = weaponarm->z + 1;
						entity->pitch = weaponarm->pitch;
					}
					else if ( entity->sprite == items[ARTIFACT_BOW].index )
					{
						entity->x = weaponarm->x - .5 * cos(weaponarm->yaw);
						entity->y = weaponarm->y - .5 * sin(weaponarm->yaw);
						entity->z = weaponarm->z + 1;
						entity->pitch = weaponarm->pitch + .25;
					}
					else if ( entity->sprite == items[TOOL_LOCKPICK].index )
					{
						entity->x = weaponarm->x + 1.5 * cos(weaponarm->yaw);
						entity->y = weaponarm->y + 1.5 * sin(weaponarm->yaw);
						entity->z = weaponarm->z + 1.5;
						entity->pitch = weaponarm->pitch + .25;
					}
					else
					{
						entity->x = weaponarm->x + .5 * cos(weaponarm->yaw) * (PLAYER_ATTACK == 0);
						entity->y = weaponarm->y + .5 * sin(weaponarm->yaw) * (PLAYER_ATTACK == 0);
						entity->z = weaponarm->z - .5 * (PLAYER_ATTACK == 0);
						entity->pitch = weaponarm->pitch + .25 * (PLAYER_ATTACK == 0);
					}
					entity->yaw = weaponarm->yaw;
					entity->roll = weaponarm->roll;
					if ( entity->sprite >= 50 && entity->sprite < 58 )
					{
						entity->roll += (PI / 2);
					}
					if ( !PLAYER_ARMBENDED )
					{
						entity->focalx = limbs[HUMAN][6][0]; // 1.5
						if ( entity->sprite == items[CROSSBOW].index )
						{
							entity->focalx += 2;
						}
						entity->focaly = limbs[HUMAN][6][1]; // 0
						entity->focalz = limbs[HUMAN][6][2]; // -.5
					}
					else
					{
						entity->focalx = limbs[HUMAN][6][0] + 1.5; // 3
						entity->focaly = limbs[HUMAN][6][1]; // 0
						entity->focalz = limbs[HUMAN][6][2] - 2; // -2.5
						entity->yaw -= sin(weaponarm->roll) * PI / 2;
						entity->pitch += cos(weaponarm->roll) * PI / 2;
					}
				}
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
				entity->x -= 2.5 * cos(my->yaw + PI / 2) + .20 * cos(my->yaw);
				entity->y -= 2.5 * sin(my->yaw + PI / 2) + .20 * sin(my->yaw);
				entity->z += 2.5;
				if ( entity->sprite == items[TOOL_TORCH].index )
				{
					entity2 = spawnFlame(entity, SPRITE_FLAME);
					if ( PLAYER_NUM == clientnum )
					{
						entity2->flags[GENIUS] = true;
					}
					entity2->x += 2 * cos(my->yaw);
					entity2->y += 2 * sin(my->yaw);
					entity2->z -= 2;
					if ( my->skill[2] == clientnum )
					{
						entity2->setUID(-4);
					}
					else
					{
						entity2->setUID(-3);
					}
				}
				else if ( entity->sprite == items[TOOL_CRYSTALSHARD].index )
				{
					entity2 = spawnFlame(entity, SPRITE_CRYSTALFLAME);
					if ( PLAYER_NUM == clientnum )
					{
						entity2->flags[GENIUS] = true;
					}
					entity2->x += 2 * cos(my->yaw);
					entity2->y += 2 * sin(my->yaw);
					entity2->z -= 2;
					if ( my->skill[2] == clientnum )
					{
						entity2->setUID(-4);
					}
					else
					{
						entity2->setUID(-3);
					}
				}
				else if ( entity->sprite == items[TOOL_LANTERN].index )
				{
					entity->z += 2;
					entity2 = spawnFlame(entity, SPRITE_FLAME);
					if ( PLAYER_NUM == clientnum )
					{
						entity2->flags[GENIUS] = true;
					}
					entity2->x += 2 * cos(my->yaw);
					entity2->y += 2 * sin(my->yaw);
					entity2->z += 1;
					if ( my->skill[2] == clientnum )
					{
						entity2->setUID(-4);
					}
					else
					{
						entity2->setUID(-3);
					}
				}
				break;
			// cloak
			case 8:
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
				entity->x -= cos(my->yaw);
				entity->y -= sin(my->yaw);
				entity->yaw += PI / 2;
				break;
			// helm
			case 9:
				entity->focalx = limbs[HUMAN][9][0]; // 0
				entity->focaly = limbs[HUMAN][9][1]; // 0
				entity->focalz = limbs[HUMAN][9][2]; // -1.75
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
				if ( entity->sprite != items[STEEL_HELM].index )
				{
					if ( entity->sprite == items[HAT_PHRYGIAN].index )
					{
						entity->focalx = limbs[HUMAN][9][0] - .5;
						entity->focaly = limbs[HUMAN][9][1] - 3.25;
						entity->focalz = limbs[HUMAN][9][2] + 2.25;
						entity->roll = PI / 2;
					}
					else if ( entity->sprite >= items[HAT_HOOD].index && entity->sprite < items[HAT_HOOD].index + items[HAT_HOOD].variations )
					{
						entity->focalx = limbs[HUMAN][9][0] - .5;
						entity->focaly = limbs[HUMAN][9][1] - 2.5;
						entity->focalz = limbs[HUMAN][9][2] + 2.25;
						entity->roll = PI / 2;
					}
					else if ( entity->sprite == items[HAT_WIZARD].index )
					{
						entity->focalx = limbs[HUMAN][9][0];
						entity->focaly = limbs[HUMAN][9][1] - 4.75;
						entity->focalz = limbs[HUMAN][9][2] + 2.25;
						entity->roll = PI / 2;
					}
					else if ( entity->sprite == items[HAT_JESTER].index )
					{
						entity->focalx = limbs[HUMAN][9][0];
						entity->focaly = limbs[HUMAN][9][1] - 4.75;
						entity->focalz = limbs[HUMAN][9][2] + 2.25;
						entity->roll = PI / 2;
					}
				}
				break;
			// mask
			case 10:
				entity->focalx = limbs[HUMAN][10][0]; // 0
				entity->focaly = limbs[HUMAN][10][1]; // 0
				entity->focalz = limbs[HUMAN][10][2]; // .5
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
				if ( entity->sprite != 165 )
				{
					entity->focalx = limbs[HUMAN][10][0] + .35; // .35
					entity->focaly = limbs[HUMAN][10][1] - 2; // -2
					entity->focalz = limbs[HUMAN][10][2]; // .5
				}
				else
				{
					entity->focalx = limbs[HUMAN][10][0] + .25; // .25
					entity->focaly = limbs[HUMAN][10][1] - 2.25; // -2.25
					entity->focalz = limbs[HUMAN][10][2]; // .5
				}
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
