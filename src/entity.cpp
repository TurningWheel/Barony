/*-------------------------------------------------------------------------------

	BARONY
	File: entity.cpp
	Desc: implements entity code

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "entity.hpp"
#include "items.hpp"
#include "monster.hpp"
#include "sound.hpp"
#include "magic/magic.hpp"
#include "interface/interface.hpp"
#include "net.hpp"
#include "collision.hpp"
#include "paths.hpp"
#ifdef STEAMWORKS
#include <steam/steam_api.h>
#endif
#include "player.hpp"
#ifdef __ARM_NEON__
#include <arm_neon.h>
#endif

/*-------------------------------------------------------------------------------

	Entity::Entity)

	Construct an Entity

-------------------------------------------------------------------------------*/

Entity::Entity(Sint32 in_sprite, Uint32 pos, list_t* entlist) :
	char_gonnavomit(skill[26]),
	char_heal(skill[22]),
	char_energize(skill[23]),
	char_torchtime(skill[25]),
	char_poison(skill[21]),
	monster_attack(skill[8]),
	monster_attacktime(skill[9]),
	monster_state(skill[0]),
	monster_target(skill[1]),
	circuit_status(skill[28]),
	switch_power(skill[0]),
	chest_status(skill[1]),
	chest_opener(skill[5])
{
	int c;
	// add the entity to the entity list
	if (!pos)
	{
		mynode = list_AddNodeFirst(entlist);
	}
	else
	{
		mynode = list_AddNodeLast(entlist);
	}
	mynode->element = this;
	mynode->deconstructor = &entityDeconstructor;
	mynode->size = sizeof(Entity);

	// now reset all of my data elements
	lastupdate = 0;
	lastupdateserver = 0;
	ticks = 0;
	x = 0;
	y = 0;
	z = 0;
	new_x = 0;
	new_y = 0;
	new_z = 0;
	focalx = 0;
	focaly = 0;
	focalz = 0;
	scalex = 1;
	scaley = 1;
	scalez = 1;
	vel_x = 0;
	vel_y = 0;
	vel_z = 0;
	sizex = 0;
	sizey = 0;
	yaw = 0;
	pitch = 0;
	roll = 0;
	new_yaw = 0;
	new_pitch = 0;
	new_roll = 0;
	sprite = in_sprite;
	light = NULL;
	string = NULL;
	children.first = NULL;
	children.last = NULL;
	//this->magic_effects = (list_t *) malloc(sizeof(list_t));
	//this->magic_effects->first = NULL; this->magic_effects->last = NULL;
	for ( c = 0; c < 30; c++ )
	{
		skill[c] = 0;
		fskill[c] = 0;
	}
	skill[2] = -1;
	for ( c = 0; c < 16; c++ )
	{
		flags[c] = false;
	}
	if ( entlist == map.entities )
	{
		if ( multiplayer != CLIENT || loading )
		{
			uid = entity_uids;
			entity_uids++;
			map.entities_map.insert({uid, mynode});
		}
		else
		{
			uid = -2;
		}
	}
	else
	{
		uid = -2;
	}
	behavior = NULL;
	ranbehavior = false;
	parent = 0;
	path = NULL;
}

void Entity::setUID(Uint32 new_uid) {
	if ( mynode->list == map.entities ) {
		map.entities_map.erase(uid);
		map.entities_map.insert({new_uid, mynode});
	}
	uid = new_uid;
}

/*-------------------------------------------------------------------------------

	Entity::~Entity)

	Deconstruct an Entity

-------------------------------------------------------------------------------*/

Entity::~Entity()
{
	node_t* node;
	//node_t *node2;
	int i;
	//deleteent_t *deleteent;

	// remove any remaining "parent" references
	/*if( entity->mynode != NULL ) {
		if( entity->mynode->list != NULL ) {
			for( node2=entity->mynode->list->first; node2!=NULL; node2=node2->next ) {
				Entity *entity2 = (Entity *)node2->element;
				if( entity2 != entity && entity2 != NULL )
					if( entity2->parent == entity )
						entity2->parent = NULL;
			}
		}
	}*/

	// alert clients of the entity's deletion
	if (multiplayer == SERVER && !loading)
	{
		if (mynode->list == map.entities && uid != 0 && flags[NOUPDATE] == false)
		{
			for (i = 1; i < MAXPLAYERS; i++)
			{
				if ( client_disconnected[i] == true )
				{
					continue;
				}

				// create a reminder for the server to continue informing the client of the deletion
				/*deleteent = (deleteent_t *) malloc(sizeof(deleteent_t)); //TODO: C++-PORT: Replace with new + class.
				deleteent->uid = uid;
				deleteent->tries = 0;
				node = list_AddNodeLast(&entitiesToDelete[i]);
				node->element = deleteent;
				node->deconstructor = &defaultDeconstructor;*/

				// send the delete entity command to the client
				strcpy((char*)net_packet->data, "ENTD");
				SDLNet_Write32((Uint32)uid, &net_packet->data[4]);
				net_packet->address.host = net_clients[i - 1].host;
				net_packet->address.port = net_clients[i - 1].port;
				net_packet->len = 8;
				/*if ( directConnect ) {
					SDLNet_UDP_Send(net_sock,-1,net_packet);
				} else {
					#ifdef STEAMWORKS
					SteamNetworking()->SendP2PPacket(*static_cast<CSteamID* >(steamIDRemote[i - 1]), net_packet->data, net_packet->len, k_EP2PSendUnreliable, 0);
					#endif
				}*/
				sendPacketSafe(net_sock, -1, net_packet, i - 1);
			}
		}
	}

	// set appropriate player pointer to NULL
	for (i = 0; i < MAXPLAYERS; i++)
		if (this == players[i]->entity)
		{
			players[i]->entity = nullptr;    //TODO: PLAYERSWAP VERIFY. Should this do anything to the player itself?
		}
	// destroy my children
	list_FreeAll(&this->children);

	node = list_AddNodeLast(&entitiesdeleted);
	node->element = this;
	node->deconstructor = &emptyDeconstructor;
}

/*-------------------------------------------------------------------------------

	Entity::setObituary

	Sets the obituary text on an entity.

-------------------------------------------------------------------------------*/

void Entity::setObituary(char* obituary)
{
	Stat* tempStats = this->getStats();
	if ( !tempStats )
	{
		return;
	}
	strncpy(tempStats->obituary, obituary, 127);
}

/*-------------------------------------------------------------------------------

	Entity::killedByMonsterObituary

	Sets the obituary to that of a mon

-------------------------------------------------------------------------------*/

void Entity::killedByMonsterObituary(Entity* victim)
{
	if ( !victim )
	{
		return;
	}
	Stat* hitstats = victim->getStats();
	Stat* myStats = this->getStats();
	if ( !hitstats || !myStats )
	{
		return;
	}

	if ( myStats->type == hitstats->type )
	{
		if ( hitstats->sex == MALE )
		{
			if ( hitstats->type < 21 ) //Original monster count
			{
				snprintf(tempstr, 256, language[1509], language[90 + hitstats->type]);
			}
			else if ( hitstats->type >= 21 ) //New monsters
			{
				snprintf(tempstr, 256, language[1509], language[2000 + (hitstats->type - 21)]);
			}
		}
		else
		{
			if ( hitstats->type < 21 ) //Original monster count
			{
				snprintf(tempstr, 256, language[1510], language[90 + hitstats->type]);
			}
			else if ( hitstats->type >= 21 ) //New monsters
			{
				snprintf(tempstr, 256, language[1510], language[2000 + (hitstats->type - 21)]);
			}
		}
		victim->setObituary(tempstr);
	}
	else
	{
		switch ( myStats->type )
		{
			case HUMAN:
				victim->setObituary(language[1511]);
				break;
			case RAT:
				victim->setObituary(language[1512]);
				break;
			case GOBLIN:
				victim->setObituary(language[1513]);
				break;
			case SLIME:
				victim->setObituary(language[1514]);
				break;
			case TROLL:
				victim->setObituary(language[1515]);
				break;
			case SPIDER:
				victim->setObituary(language[1516]);
				break;
			case GHOUL:
				victim->setObituary(language[1517]);
				break;
			case SKELETON:
				victim->setObituary(language[1518]);
				break;
			case SCORPION:
				victim->setObituary(language[1519]);
				break;
			case CREATURE_IMP:
				victim->setObituary(language[1520]);
				break;
			case GNOME:
				victim->setObituary(language[1521]);
				break;
			case DEMON:
				victim->setObituary(language[1522]);
				break;
			case SUCCUBUS:
				victim->setObituary(language[1523]);
				break;
			case LICH:
				victim->setObituary(language[1524]);
				break;
			case MINOTAUR:
				victim->setObituary(language[1525]);
				break;
			case DEVIL:
				victim->setObituary(language[1526]);
				break;
			case SHOPKEEPER:
				victim->setObituary(language[1527]);
				break;
			case KOBOLD:
				victim->setObituary(language[2150]);
				break;
			case SCARAB:
				victim->setObituary(language[2151]);
				break;
			case CRYSTALGOLEM:
				victim->setObituary(language[2152]);
				break;
			case INCUBUS:
				victim->setObituary(language[2153]);
				break;
			case VAMPIRE:
				victim->setObituary(language[2154]);
				break;
			case SHADOW:
				victim->setObituary(language[2155]);
				break;
			case COCKATRICE:
				victim->setObituary(language[2156]);
				break;
			case INSECTOID:
				victim->setObituary(language[2157]);
				break;
			case GOATMAN:
				victim->setObituary(language[2158]);
				break;
			case AUTOMATON:
				victim->setObituary(language[2159]);
				break;
			case LICH_ICE:
				victim->setObituary(language[2160]);
				break;
			case LICH_FIRE:
				victim->setObituary(language[2161]);
				break;
			default:
				victim->setObituary(language[1500]);
				break;
		}
	}
}

/*-------------------------------------------------------------------------------

	Entity::light

	Returns the illumination of the given entity

-------------------------------------------------------------------------------*/

int Entity::entityLight()
{
	if ( this->flags[BRIGHT] )
	{
		return 255;
	}
	if ( this->x < 0 || this->y < 0 || this->x >= map.width << 4 || this->y >= map.height << 4 )
	{
		return 255;
	}
	int light_x = (int)this->x / 16;
	int light_y = (int)this->y / 16;
	return lightmap[light_y + light_x * map.height];
}

/*-------------------------------------------------------------------------------

	Entity::effectTimes

	Counts down effect timers and toggles effects whose timers reach zero

-------------------------------------------------------------------------------*/

void Entity::effectTimes()
{
	Stat* myStats = this->getStats();
	int player, c;
	spell_t* spell = NULL;
	node_t* node = NULL;
	int count = 0;

	if ( myStats == NULL )
	{
		return;
	}
	if ( this->behavior == &actPlayer )
	{
		player = this->skill[2];
	}
	else
	{
		player = -1;
	}


	spell_t* invisibility_hijacked = NULL; //If NULL, function proceeds as normal. If points to something, it ignores the invisibility timer since a spell is doing things.
	spell_t* levitation_hijacked = NULL; //If NULL, function proceeds as normal. If points to something, it ignore the levitation timer since a spell is doing things.
	//Handle magic effects (like invisibility)
	for (node = myStats->magic_effects.first; node; node = node->next, ++count)
	{
		//printlog( "%s\n", "Potato.");
		//Handle magic effects.
		spell = (spell_t*) node->element;
		if (!spell->sustain)
		{
			node_t* temp = NULL;
			if (node->prev)
			{
				temp = node->prev;
			}
			else if (node->next)
			{
				temp = node->next;
			}
			spell->magic_effects_node = NULL; //To prevent recursive removal, which results in a crash.
			if (player > -1 && multiplayer == SERVER)
			{
				strcpy( (char*)net_packet->data, "UNCH");
				net_packet->data[4] = player;
				SDLNet_Write32(spell->ID, &net_packet->data[5]);
				net_packet->address.host = net_clients[player - 1].host;
				net_packet->address.port = net_clients[player - 1].port;
				net_packet->len = 9;
				sendPacketSafe(net_sock, -1, net_packet, player - 1);
			}
			list_RemoveNode(node); //Bugger the spell.
			node = temp;
			if (!node)
			{
				break;    //Done with list. Stop.
			}
			continue; //Skip this spell.
		}
		switch (spell->ID)
		{
			case SPELL_INVISIBILITY:
				invisibility_hijacked = spell;
				if (!myStats->EFFECTS[EFF_INVISIBLE])
				{
					for (c = 0; c < numplayers; ++c)
					{
						if (players[c] && players[c]->entity == uidToEntity(spell->caster) && players[c]->entity != nullptr)
						{
							messagePlayer(c, language[591]);    //If cure ailments or somesuch bombs the status effects.
						}
					}
					node_t* temp = nullptr;
					if (node->prev)
					{
						temp = node->prev;
					}
					else if (node->next)
					{
						temp = node->next;
					}
					list_RemoveNode(node); //Remove this here node.
					node = temp;
				}
				break;
			case SPELL_LEVITATION:
				levitation_hijacked = spell;
				if (!myStats->EFFECTS[EFF_LEVITATING])
				{
					for (c = 0; c < numplayers; ++c)
					{
						if (players[c] && players[c]->entity == uidToEntity(spell->caster) && players[c]->entity != nullptr)
						{
							messagePlayer(c, language[592]);
						}
					}
					node_t* temp = nullptr;
					if (node->prev)
					{
						temp = node->prev;
					}
					else if (node->next)
					{
						temp = node->next;
					}
					list_RemoveNode(node); //Remove this here node.
					node = temp;
				}
				break;
			default:
				//Unknown spell, undefined effect. Like, say, a fireball spell wound up in here for some reason. That's a nono.
				printlog( "[entityEffectTimes] Warning: magic_effects spell that's not relevant. Should not be in the magic_effects list!\n");
				list_RemoveNode(node);
		}

		if (!node)
		{
			break;    //BREAK OUT. YEAAAAAH. Because otherwise it crashes.
		}
	}
	if (count)
	{
		//printlog( "Number of magic effects spells: %d\n", count); //Debugging output.
	}

	bool dissipate = true;

	for ( c = 0; c < NUMEFFECTS; c++ )
	{
		if ( myStats->EFFECTS_TIMERS[c] > 0 )
		{
			myStats->EFFECTS_TIMERS[c]--;
			if ( myStats->EFFECTS_TIMERS[c] == 0 )
			{
				myStats->EFFECTS[c] = false;
				switch ( c )
				{
					case EFF_ASLEEP:
						messagePlayer(player, language[593]);
						break;
					case EFF_POISONED:
						messagePlayer(player, language[594]);
						break;
					case EFF_STUNNED:
						messagePlayer(player, language[595]);
						break;
					case EFF_CONFUSED:
						messagePlayer(player, language[596]);
						break;
					case EFF_DRUNK:
						messagePlayer(player, language[597]);
						break;
					case EFF_INVISIBLE:
						; //To make the compiler shut up: "error: a label can only be part of a statement and a declaration is not a statement"
						dissipate = true; //Remove the effect by default.
						if (invisibility_hijacked)
						{
							bool sustained = false;
							Entity* caster = uidToEntity(invisibility_hijacked->caster);
							if (caster)
							{
								//Deduct mana from caster. Cancel spell if not enough mana (simply leave sustained at false).
								bool deducted = caster->safeConsumeMP(1); //Consume 1 mana ever duration / mana seconds
								if (deducted)
								{
									sustained = true;
									myStats->EFFECTS[c] = true;
									myStats->EFFECTS_TIMERS[c] = invisibility_hijacked->channel_duration / getCostOfSpell(invisibility_hijacked);
								}
								else
								{
									int i = 0;
									for (i = 0; i < 4; ++i)
									{
										if (players[i]->entity == caster)
										{
											messagePlayer(i, language[598]);    //TODO: Unhardcode name?
										}
									}
									list_RemoveNode(invisibility_hijacked->magic_effects_node); //Remove it from the entity's magic effects. This has the side effect of removing it from the sustained spells list too.
									//list_RemoveNode(invisibility_hijacked->sustain_node); //Remove it from the channeled spells list.
								}
							}
							if (sustained)
							{
								dissipate = false;    //Sustained the spell, so do not stop being invisible.
							}
						}
						if (dissipate)
						{
							if ( !this->isBlind() )
							{
								messagePlayer(player, language[599]);
							}
						}
						break;
					case EFF_BLIND:
						if ( !this->isBlind())
						{
							messagePlayer(player, language[600]);
						}
						else
						{
							messagePlayer(player, language[601]);
						}
						break;
					case EFF_GREASY:
						messagePlayer(player, language[602]);
						break;
					case EFF_MESSY:
						messagePlayer(player, language[603]);
						break;
					case EFF_FAST:
						messagePlayer(player, language[604]);
						break;
					case EFF_PARALYZED:
						messagePlayer(player, language[605]);
						break;
					case EFF_LEVITATING:
						; //To make the compiler shut up: "error: a label can only be part of a statement and a declaration is not a statement"
						dissipate = true; //Remove the effect by default.
						if (levitation_hijacked)
						{
							bool sustained = false;
							Entity* caster = uidToEntity(levitation_hijacked->caster);
							if (caster)
							{
								//Deduct mana from caster. Cancel spell if not enough mana (simply leave sustained at false).
								bool deducted = caster->safeConsumeMP(1); //Consume 1 mana ever duration / mana seconds
								if (deducted)
								{
									sustained = true;
									myStats->EFFECTS[c] = true;
									myStats->EFFECTS_TIMERS[c] = levitation_hijacked->channel_duration / getCostOfSpell(levitation_hijacked);
								}
								else
								{
									int i = 0;
									for (i = 0; i < 4; ++i)
									{
										if (players[i]->entity == caster)
										{
											messagePlayer(i, language[606]);    //TODO: Unhardcode name?
										}
									}
									list_RemoveNode(levitation_hijacked->magic_effects_node); //Remove it from the entity's magic effects. This has the side effect of removing it from the sustained spells list too.
								}
							}
							if (sustained)
							{
								dissipate = false;    //Sustained the spell, so do not stop levitating.
							}
						}
						if (dissipate)
						{
							messagePlayer(player, language[607]);
						}
						break;
					case EFF_TELEPATH:
						messagePlayer(player, language[608]);
						break;
					case EFF_VOMITING:
						messagePlayer(player, language[609]);
						if ( myStats->HUNGER > 1500 )
						{
							messagePlayer(player, language[610]);
						}
						else if ( myStats->HUNGER > 150 && myStats->HUNGER <= 250 )
						{
							messagePlayer(player, language[611]);
							playSoundPlayer(player, 32, 128);
						}
						else if ( myStats->HUNGER > 50 )
						{
							messagePlayer(player, language[612]);
							playSoundPlayer(player, 32, 128);
						}
						else
						{
							myStats->HUNGER = 50;
							messagePlayer(player, language[613]);
							playSoundPlayer(player, 32, 128);
						}
						serverUpdateHunger(player);
						break;
					case EFF_BLEEDING:
						messagePlayer(player, language[614]);
						break;
					default:
						break;
				}
				if ( player > 0 && multiplayer == SERVER )
				{
					serverUpdateEffects(player);
				}
			}
		}
	}
}

/*-------------------------------------------------------------------------------

	Entity::increaseSkill

	Increases the given skill of the given entity by 1.

-------------------------------------------------------------------------------*/

void Entity::increaseSkill(int skill)
{
	Stat* myStats = this->getStats();
	int player = -1;

	if ( myStats == NULL )
	{
		return;
	}
	if ( this->behavior == &actPlayer )
	{
		player = this->skill[2];
	}

	Uint32 color = SDL_MapRGB(mainsurface->format, 255, 255, 0);
	if ( myStats->PROFICIENCIES[skill] < 100 )
	{
		myStats->PROFICIENCIES[skill]++;
		messagePlayerColor(player, color, language[615], language[236 + skill]);
		switch ( myStats->PROFICIENCIES[skill] )
		{
			case 20:
				messagePlayerColor(player, color, language[616], language[236 + skill]);
				break;
			case 40:
				messagePlayerColor(player, color, language[617], language[236 + skill]);
				break;
			case 60:
				messagePlayerColor(player, color, language[618], language[236 + skill]);
				break;
			case 80:
				messagePlayerColor(player, color, language[619], language[236 + skill]);
				break;
			case 100:
				messagePlayerColor(player, color, language[620], language[236 + skill]);
				break;
			default:
				break;
		}
	}
	myStats->EXP += 2;
	if ( player > 0 && multiplayer == SERVER )
	{
		// update SKILL
		strcpy((char*)net_packet->data, "SKIL");
		net_packet->data[4] = clientnum;
		net_packet->data[5] = skill;
		net_packet->data[6] = myStats->PROFICIENCIES[skill];
		net_packet->address.host = net_clients[player - 1].host;
		net_packet->address.port = net_clients[player - 1].port;
		net_packet->len = 7;
		sendPacketSafe(net_sock, -1, net_packet, player - 1);

		// update EXP
		strcpy((char*)net_packet->data, "ATTR");
		net_packet->data[4] = clientnum;
		net_packet->data[5] = (Sint8)myStats->STR;
		net_packet->data[6] = (Sint8)myStats->DEX;
		net_packet->data[7] = (Sint8)myStats->CON;
		net_packet->data[8] = (Sint8)myStats->INT;
		net_packet->data[9] = (Sint8)myStats->PER;
		net_packet->data[10] = (Sint8)myStats->CHR;
		net_packet->data[11] = (Sint8)myStats->EXP;
		net_packet->data[12] = (Sint8)myStats->LVL;
		SDLNet_Write16((Sint16)myStats->HP, &net_packet->data[13]);
		SDLNet_Write16((Sint16)myStats->MAXHP, &net_packet->data[15]);
		SDLNet_Write16((Sint16)myStats->MP, &net_packet->data[17]);
		SDLNet_Write16((Sint16)myStats->MAXMP, &net_packet->data[19]);
		net_packet->address.host = net_clients[player - 1].host;
		net_packet->address.port = net_clients[player - 1].port;
		net_packet->len = 21;
		sendPacketSafe(net_sock, -1, net_packet, player - 1);
	}
}

/*-------------------------------------------------------------------------------

	Entity::stats

	Returns a pointer to a Stat instance given a pointer to an entity

-------------------------------------------------------------------------------*/

Stat* Entity::getStats()
{
	if (this->behavior == &actMonster)   // monsters
	{
		if (this->children.first != nullptr)
		{
			if (this->children.first->next != nullptr)
			{
				return (Stat*)this->children.first->next->element;
			}
		}
	}
	else if (this->behavior == &actPlayer)     // players
	{
		return stats[this->skill[2]];
	}
	else if (this->behavior == &actPlayerLimb)     // player bodyparts
	{
		return stats[this->skill[2]];
	}

	return nullptr;
}

/*-------------------------------------------------------------------------------

	Entity::checkBetterEquipment

	Checks the tiles immediately surrounding the given entity for items and
	replaces the entity's equipment with those items if they are better

-------------------------------------------------------------------------------*/

void Entity::checkBetterEquipment(Stat* myStats)
{
	if (!myStats)
	{
		return;    //Can't continue without these.
	}

	list_t* items = NULL;
	//X and Y in terms of tiles.
	int tx = x / 16;
	int ty = y / 16;
	getItemsOnTile(tx, ty, &items); //Check the tile the goblin is on for items.
	getItemsOnTile(tx - 1, ty, &items); //Check tile to the left.
	getItemsOnTile(tx + 1, ty, &items); //Check tile to the right.
	getItemsOnTile(tx, ty - 1, &items); //Check tile up.
	getItemsOnTile(tx, ty + 1, &items); //Check tile down.
	getItemsOnTile(tx - 1, ty - 1, &items); //Check tile diagonal up left.
	getItemsOnTile(tx + 1, ty - 1, &items); //Check tile diagonal up right.
	getItemsOnTile(tx - 1, ty + 1, &items); //Check tile diagonal down left.
	getItemsOnTile(tx + 1, ty + 1, &items); //Check tile diagonal down right.
	int currentAC, newAC;
	Item* oldarmor = NULL;

	node_t* node = NULL;

	bool glovesandshoes = false;
	if ( myStats->type == HUMAN )
	{
		glovesandshoes = true;
	}

	if (items)
	{
		/*
		 * Rundown of the function:
		 * Loop through all items.
		 * Check the monster's item. Compare and grab the best item.
		 */

		for (node = items->first; node != NULL; node = node->next)
		{
			//Turn the entity into an item.
			if (node->element)
			{
				Entity* entity = (Entity*)node->element;
				Item* item = NULL;
				if (entity != NULL)
				{
					item = newItemFromEntity( entity );
				}

				if (item != NULL)
				{
					//If weapon.
					if (itemCategory(item) == WEAPON)
					{
						if (myStats->weapon == NULL)   //Not currently holding a weapon.
						{
							myStats->weapon = item; //Assign the monster's weapon.
							item = NULL;
							list_RemoveNode(entity->mynode);
						}
						else
						{
							//Ok, the monster has a weapon already. First check if the monster's weapon is cursed. Can't drop it if it is.
							if (myStats->weapon->beatitude >= 0 && itemCategory(myStats->weapon) != MAGICSTAFF)
							{
								//Next compare the two weapons. If the item on the ground is better, drop the weapon it's carrying and equip that one.
								int weapon_tohit = myStats->weapon->weaponGetAttack();
								int new_weapon_tohit = item->weaponGetAttack();

								//If the new weapon does more damage than the current weapon.
								if (new_weapon_tohit > weapon_tohit)
								{
									dropItemMonster(myStats->weapon, this, myStats);
									myStats->weapon = item;
									item = NULL;
									list_RemoveNode(entity->mynode);
								}
							}
						}
					}
					else if ( itemCategory(item) == ARMOR )
					{
						if ( item->type == HAT_PHRYGIAN || item->type == HAT_WIZARD || item->type == HAT_JESTER || item->type == HAT_HOOD )   // hats
						{
							if (myStats->helmet == NULL)   // nothing on head currently
							{
								// goblins love hats.
								myStats->helmet = item; // pick up the hat.
								item = NULL;
								list_RemoveNode(entity->mynode);
							}
						}
						else if ( item->type == LEATHER_HELM || item->type == IRON_HELM || item->type == STEEL_HELM )     // helmets
						{
							if (myStats->helmet == NULL)   // nothing on head currently
							{
								myStats->helmet = item; // pick up the helmet.
								item = NULL;
								list_RemoveNode(entity->mynode);
							}
							else
							{
								if (myStats->helmet->beatitude >= 0)   // if the armor is not cursed, proceed. Won't do anything if the armor is cursed.
								{
									// to compare the armors, we use the AC function to check the Armor Class of the equipment the goblin
									// is currently wearing versus the Armor Class that the goblin would have if it had the new armor.
									currentAC = AC(myStats);
									oldarmor = myStats->helmet;
									myStats->helmet = item;
									newAC = AC(myStats);
									myStats->helmet = oldarmor;

									//If the new armor is better than the current armor.
									if (newAC > currentAC)
									{
										dropItemMonster(myStats->helmet, this, myStats);
										myStats->helmet = item;
										item = NULL;
										list_RemoveNode(entity->mynode);
									}
								}
							}
						}
						else if ( item->type == WOODEN_SHIELD || item->type == BRONZE_SHIELD || item->type == IRON_SHIELD || item->type == STEEL_SHIELD || item->type == STEEL_SHIELD_RESISTANCE || item->type == TOOL_TORCH || item->type == TOOL_LANTERN )     // shields
						{
							if (myStats->shield == NULL)   // nothing in left hand currently
							{
								myStats->shield = item; // pick up the shield.
								item = NULL;
								list_RemoveNode(entity->mynode);
							}
							else
							{
								if (myStats->shield->beatitude >= 0)   // if the armor is not cursed, proceed. Won't do anything if the armor is cursed.
								{
									// to compare the armors, we use the AC function to check the Armor Class of the equipment the goblin
									// is currently wearing versus the Armor Class that the goblin would have if it had the new armor.
									currentAC = AC(myStats);
									oldarmor = myStats->shield;
									myStats->shield = item;
									newAC = AC(myStats);
									myStats->shield = oldarmor;

									//If the new armor is better than the current armor (OR we're not carrying anything)
									if (newAC > currentAC || !myStats->shield )
									{
										dropItemMonster(myStats->shield, this, myStats);
										myStats->shield = item;
										item = NULL;
										list_RemoveNode(entity->mynode);
									}
								}
							}
						}
						else if ( item->type == LEATHER_BREASTPIECE || item->type == IRON_BREASTPIECE || item->type == STEEL_BREASTPIECE )     // breastpieces
						{
							if (myStats->breastplate == NULL)   // nothing on torso currently
							{
								myStats->breastplate = item; // pick up the armor.
								item = NULL;
								list_RemoveNode(entity->mynode);
							}
							else
							{
								if (myStats->breastplate->beatitude >= 0)   // if the armor is not cursed, proceed. Won't do anything if the armor is cursed.
								{
									// to compare the armors, we use the AC function to check the Armor Class of the equipment the goblin
									// is currently wearing versus the Armor Class that the goblin would have if it had the new armor.
									currentAC = AC(myStats);
									oldarmor = myStats->breastplate;
									myStats->breastplate = item;
									newAC = AC(myStats);
									myStats->breastplate = oldarmor;

									//If the new armor is better than the current armor.
									if (newAC > currentAC)
									{
										dropItemMonster(myStats->breastplate, this, myStats);
										myStats->breastplate = item;
										item = NULL;
										list_RemoveNode(entity->mynode);
									}
								}
							}
						}
						else if ( item->type == CLOAK || item->type == CLOAK_MAGICREFLECTION || item->type == CLOAK_INVISIBILITY || item->type == CLOAK_PROTECTION )     // cloaks
						{
							if (myStats->cloak == NULL)   // nothing on back currently
							{
								myStats->cloak = item; // pick up the armor.
								item = NULL;
								list_RemoveNode(entity->mynode);
							}
							else
							{
								if (myStats->cloak->beatitude >= 0)   // if the armor is not cursed, proceed. Won't do anything if the armor is cursed.
								{
									// to compare the armors, we use the AC function to check the Armor Class of the equipment the goblin
									// is currently wearing versus the Armor Class that the goblin would have if it had the new armor.
									currentAC = AC(myStats);
									oldarmor = myStats->cloak;
									myStats->cloak = item;
									newAC = AC(myStats);
									myStats->cloak = oldarmor;

									//If the new armor is better than the current armor.
									if (newAC > currentAC)
									{
										dropItemMonster(myStats->cloak, this, myStats);
										myStats->cloak = item;
										item = NULL;
										list_RemoveNode(entity->mynode);
									}
								}
							}
						}
						if ( glovesandshoes && item != NULL )
						{
							if ( item->type >= LEATHER_BOOTS && item->type <= STEEL_BOOTS_FEATHER )   // boots
							{
								if (myStats->shoes == NULL)
								{
									myStats->shoes = item; // pick up the armor
									item = NULL;
									list_RemoveNode(entity->mynode);
								}
								else
								{
									if (myStats->shoes->beatitude >= 0)   // if the armor is not cursed, proceed. Won't do anything if the armor is cursed.
									{
										// to compare the armors, we use the AC function to check the Armor Class of the equipment the goblin
										// is currently wearing versus the Armor Class that the goblin would have if it had the new armor.
										currentAC = AC(myStats);
										oldarmor = myStats->shoes;
										myStats->shoes = item;
										newAC = AC(myStats);
										myStats->shoes = oldarmor;

										//If the new armor is better than the current armor.
										if (newAC > currentAC)
										{
											dropItemMonster(myStats->shoes, this, myStats);
											myStats->shoes = item;
											item = NULL;
											list_RemoveNode(entity->mynode);
										}
									}
								}
							}
							else if ( item->type >= GLOVES && item->type <= GAUNTLETS_STRENGTH )
							{
								if (myStats->gloves == NULL)
								{
									myStats->gloves = item; // pick up the armor
									item = NULL;
									list_RemoveNode(entity->mynode);
								}
								else
								{
									if (myStats->gloves->beatitude >= 0)   // if the armor is not cursed, proceed. Won't do anything if the armor is cursed.
									{
										// to compare the armors, we use the AC function to check the Armor Class of the equipment the goblin
										// is currently wearing versus the Armor Class that the goblin would have if it had the new armor.
										currentAC = AC(myStats);
										oldarmor = myStats->gloves;
										myStats->gloves = item;
										newAC = AC(myStats);
										myStats->gloves = oldarmor;

										//If the new armor is better than the current armor.
										if (newAC > currentAC)
										{
											dropItemMonster(myStats->gloves, this, myStats);
											myStats->gloves = item;
											item = NULL;
											list_RemoveNode(entity->mynode);
										}
									}
								}
							}
						}
					}
				}

				if (item != NULL)
				{
					free(item);
				}
			}
		}

		list_FreeAll(items);
		free(items);
	}
}

/*-------------------------------------------------------------------------------

	uidToEntity

	Returns an entity pointer from the given entity UID, provided one exists.
	Otherwise returns NULL

-------------------------------------------------------------------------------*/

Entity* uidToEntity(Sint32 uidnum)
{
	node_t* node;
	Entity* entity;

	auto it = map.entities_map.find(uidnum);
	if(it != map.entities_map.end())
		return (Entity*)it->second->element;

	return NULL;
}

/*-------------------------------------------------------------------------------

	Entity::setHP

	sets the HP of the given entity

-------------------------------------------------------------------------------*/

void Entity::setHP(int amount)
{
	Stat* entitystats = this->getStats();

	if (this->behavior == &actPlayer && godmode)
	{
		amount = entitystats->MAXHP;
	}
	if (!entitystats || amount == entitystats->HP)
	{
		return;
	}
	entitystats->HP = std::min(std::max(0, amount), entitystats->MAXHP);
	strncpy(entitystats->obituary, language[1500], 127);

	int i = 0;
	if (multiplayer == SERVER)
	{
		for (i = 1; i < numplayers; i++)
		{
			if (this == players[i]->entity)
			{
				// tell the client its HP changed
				strcpy((char*)net_packet->data, "UPHP");
				SDLNet_Write32((Uint32)entitystats->HP, &net_packet->data[4]);
				SDLNet_Write32((Uint32)NOTHING, &net_packet->data[8]);
				net_packet->address.host = net_clients[i - 1].host;
				net_packet->address.port = net_clients[i - 1].port;
				net_packet->len = 12;
				sendPacketSafe(net_sock, -1, net_packet, i - 1);
			}
		}
	}
}

/*-------------------------------------------------------------------------------

	Entity::modHP

	modifies the HP of the given entity

-------------------------------------------------------------------------------*/

void Entity::modHP(int amount)
{
	Stat* entitystats = this->getStats();

	if ( this->behavior == &actPlayer && godmode && amount < 0 )
	{
		amount = 0;
	}
	if ( !entitystats || amount == 0 )
	{
		return;
	}

	this->setHP(entitystats->HP + amount);
}

/*-------------------------------------------------------------------------------

	Entity::setMP

	sets the MP of the given entity

-------------------------------------------------------------------------------*/

void Entity::setMP(int amount)
{
	Stat* entitystats = this->getStats();

	if (this->behavior == &actPlayer && godmode)
	{
		amount = entitystats->MAXMP;
	}
	if (!entitystats || amount == entitystats->MP)
	{
		return;
	}
	entitystats->MP = std::min(std::max(0, amount), entitystats->MAXMP);

	int i = 0;
	if (multiplayer == SERVER)
	{
		for (i = 1; i < numplayers; i++)
		{
			if (this == players[i]->entity)
			{
				// tell the client its MP just changed
				strcpy((char*)net_packet->data, "UPMP");
				SDLNet_Write32((Uint32)entitystats->MP, &net_packet->data[4]);
				net_packet->address.host = net_clients[i - 1].host;
				net_packet->address.port = net_clients[i - 1].port;
				net_packet->len = 8;
				sendPacketSafe(net_sock, -1, net_packet, i - 1);
			}
		}
	}
}

/*-------------------------------------------------------------------------------

	Entity::modMP

	modifies the MP of the given entity

-------------------------------------------------------------------------------*/

void Entity::modMP(int amount)
{
	Stat* entitystats = this->getStats();

	if ( this->behavior == &actPlayer && godmode && amount < 0 )
	{
		amount = 0;
	}
	if ( !entitystats || amount == 0 )
	{
		return;
	}

	this->setMP(entitystats->MP + amount);
}

/*-------------------------------------------------------------------------------

	Entity::drainMP

	 Removes this much from MP. Anything over the entity's MP is subtracted from their health. Can be very dangerous.

-------------------------------------------------------------------------------*/

void Entity::drainMP(int amount)
{
	//A pointer to the entity's stats.
	Stat* entitystats = this->getStats();

	//Check if no stats found.
	if (entitystats == NULL || amount == 0)
	{
		return;
	}

	int overdrawn = 0;
	entitystats->MP -= amount;
	int player = -1;
	int i = 0;
	for (i = 0; i < numplayers; ++i)
	{
		if (this == players[i]->entity)
		{
			player = i; //Set the player.
		}
	}
	if (entitystats->MP < 0)
	{
		//Overdrew. Take that extra and flow it over into HP.
		overdrawn = entitystats->MP;
		entitystats->MP = 0;
	}
	if (multiplayer == SERVER)
	{
		//First check if the entity is the player.
		for (i = 1; i < numplayers; ++i)
		{
			if (this == players[i]->entity)
			{
				//It is. Tell the client its MP just changed.
				strcpy((char*)net_packet->data, "UPMP");
				SDLNet_Write32((Uint32)entitystats->MP, &net_packet->data[4]);
				SDLNet_Write32((Uint32)stats[i]->type, &net_packet->data[8]);
				net_packet->address.host = net_clients[i - 1].host;
				net_packet->address.port = net_clients[i - 1].port;
				net_packet->len = 12;
				sendPacketSafe(net_sock, -1, net_packet, i - 1);
			}
		}
	}
	else if (clientnum != 0 && multiplayer == CLIENT)
	{
		if (this == players[clientnum]->entity)
		{
			//It's the player entity. Tell the server its MP changed.
			strcpy((char*)net_packet->data, "UPMP");
			net_packet->data[4] = clientnum;
			SDLNet_Write32((Uint32)entitystats->MP, &net_packet->data[5]);
			SDLNet_Write32((Uint32)stats[clientnum]->type, &net_packet->data[9]);
			net_packet->address.host = net_server.host;
			net_packet->address.port = net_server.port;
			net_packet->len = 13;
			sendPacketSafe(net_sock, -1, net_packet, 0);
		}
	}

	if (overdrawn < 0)
	{
		if (player >= 0)
		{
			Uint32 color = SDL_MapRGB(mainsurface->format, 255, 255, 0);
			messagePlayerColor(player, color, language[621]);
		}
		this->modHP(overdrawn); //Drain the extra magic from health.
		Stat* tempStats = this->getStats();
		if ( tempStats )
		{
			if ( tempStats->sex == MALE )
			{
				this->setObituary(language[1528]);
			}
			else
			{
				this->setObituary(language[1529]);
			}
		}
	}
}

/*-------------------------------------------------------------------------------

	Entity::safeConsumeMP

	A function for the magic code. Attempts to remove mana without overdrawing the player. Returns true if success, returns false if didn't have enough mana.

-------------------------------------------------------------------------------*/

bool Entity::safeConsumeMP(int amount)
{
	Stat* stat = this->getStats();

	//Check if no stats found.
	if (!stat)
	{
		return false;
	}

	if (amount > stat->MP)
	{
		return false;    //Not enough mana.
	}
	else
	{
		this->modMP(-amount);
		return true;
	}

	return false;
}

/*-------------------------------------------------------------------------------

	Entity::handleEffects

	processes general character status updates for a given entity, such as
	hunger, level ups, poison, etc.

-------------------------------------------------------------------------------*/

void Entity::handleEffects(Stat* myStats)
{
	int increasestat[3];
	int i, c;
	int player = -1;

	if ( this->behavior == &actPlayer )
	{
		player = this->skill[2];

		// god mode and buddha mode
		if ( godmode )
		{
			myStats->HP = myStats->MAXHP;
			myStats->MP = myStats->MAXMP;
		}
		else if ( buddhamode )
		{
			if ( myStats->HP <= 0 )
			{
				myStats->HP = 1;
			}
		}
	}

	// sleep Zs
	if ( myStats->EFFECTS[EFF_ASLEEP] && ticks % 30 == 0 )
	{
		spawnSleepZ(this->x + cos(this->yaw) * 2, this->y + sin(this->yaw) * 2, this->z);
	}

	// level ups
	if ( myStats->EXP >= 100 )
	{
		myStats->EXP -= 100;
		myStats->LVL++;
		Uint32 color = SDL_MapRGB(mainsurface->format, 255, 255, 0);
		messagePlayerColor(player, color, language[622]);
		playSoundPlayer(player, 97, 128);

		// increase MAXHP/MAXMP
		myStats->HP += HP_MOD;
		myStats->MAXHP += HP_MOD;
		myStats->HP = std::min(myStats->HP, myStats->MAXHP);
		myStats->MP += MP_MOD;
		myStats->MAXMP += MP_MOD;
		myStats->MP = std::min(myStats->MP, myStats->MAXMP);

		// now pick three attributes to increase
		increasestat[0] = rand() % 6;
		increasestat[1] = rand() % 5;
		increasestat[2] = rand() % 4;
		if ( increasestat[1] >= increasestat[0] )
		{
			increasestat[1]++;
		}
		if ( increasestat[2] >= increasestat[0] )
		{
			increasestat[2]++;
		}
		if ( increasestat[2] >= increasestat[1] )
		{
			increasestat[2]++;
		}
		for ( i = 0; i < 3; i++ )
		{
			messagePlayerColor(player, color, language[623 + increasestat[i]]);
			switch ( increasestat[i] )
			{
				case 0: // STR
					myStats->STR++;
					break;
				case 1: // DEX
					myStats->DEX++;
					break;
				case 2: // CON
					myStats->CON++;
					break;
				case 3: // INT
					myStats->INT++;
					break;
				case 4: // PER
					myStats->PER++;
					break;
				case 5: // CHR
					myStats->CHR++;
					break;
			}
		}

		// inform clients of stat changes
		if ( multiplayer == SERVER && player > 0 )
		{
			strcpy((char*)net_packet->data, "ATTR");
			net_packet->data[4] = clientnum;
			net_packet->data[5] = (Sint8)myStats->STR;
			net_packet->data[6] = (Sint8)myStats->DEX;
			net_packet->data[7] = (Sint8)myStats->CON;
			net_packet->data[8] = (Sint8)myStats->INT;
			net_packet->data[9] = (Sint8)myStats->PER;
			net_packet->data[10] = (Sint8)myStats->CHR;
			net_packet->data[11] = (Sint8)myStats->EXP;
			net_packet->data[12] = (Sint8)myStats->LVL;
			SDLNet_Write16((Sint16)myStats->HP, &net_packet->data[13]);
			SDLNet_Write16((Sint16)myStats->MAXHP, &net_packet->data[15]);
			SDLNet_Write16((Sint16)myStats->MP, &net_packet->data[17]);
			SDLNet_Write16((Sint16)myStats->MAXMP, &net_packet->data[19]);
			net_packet->address.host = net_clients[player - 1].host;
			net_packet->address.port = net_clients[player - 1].port;
			net_packet->len = 21;
			sendPacketSafe(net_sock, -1, net_packet, player - 1);
		}
	}

	// hunger
	int hungerring = 0;
	if ( myStats->ring != NULL )
	{
		if ( myStats->ring->type == RING_SLOWDIGESTION )
		{
			if ( myStats->ring->beatitude >= 0 )
			{
				hungerring = 1;
			}
			else
			{
				hungerring = -1;
			}
		}
	}
	if ( (ticks % 30 == 0 && !hungerring) || (ticks % 15 == 0 && hungerring < 0) || (ticks % 120 == 0 && hungerring > 0) )
	{
		if ( myStats->HUNGER > 0 )
		{
			if ( svFlags & SV_FLAG_HUNGER )
			{
				myStats->HUNGER--;
			}
			else if ( myStats->HUNGER != 800 )
			{
				myStats->HUNGER = 800;
				serverUpdateHunger(player);
			}
			if ( myStats->HUNGER == 1500 )
			{
				if ( !myStats->EFFECTS[EFF_VOMITING] )
				{
					messagePlayer(player, language[629]);
				}
				serverUpdateHunger(player);
			}
			else if ( myStats->HUNGER == 250 )
			{
				if ( !myStats->EFFECTS[EFF_VOMITING] )
				{
					messagePlayer(player, language[630]);
					playSoundPlayer(player, 32, 128);
				}
				serverUpdateHunger(player);
			}
			else if ( myStats->HUNGER == 150 )
			{
				if ( !myStats->EFFECTS[EFF_VOMITING] )
				{
					messagePlayer(player, language[631]);
					playSoundPlayer(player, 32, 128);
				}
				serverUpdateHunger(player);
			}
			else if ( myStats->HUNGER == 50 )
			{
				if ( !myStats->EFFECTS[EFF_VOMITING] )
				{
					messagePlayer(player, language[632]);
					playSoundPlayer(player, 32, 128);
				}
				serverUpdateHunger(player);
			}
		}
		else
		{
			myStats->HUNGER = 0;
			if ( !myStats->EFFECTS[EFF_VOMITING] && ticks % 120 == 0 )
			{
				serverUpdateHunger(player);
				if ( player >= 0 )   // bad guys don't starve. Sorry.
				{
					this->modHP(-4);
				}
				if ( myStats->HP > 0 )
				{
					messagePlayer(player, language[633]);
				}
				this->setObituary(language[1530]);
			}
		}
	}

	// "random" vomiting
	if ( !this->char_gonnavomit && !myStats->EFFECTS[EFF_VOMITING] )
	{
		if ( myStats->HUNGER > 1500 && rand() % 1000 == 0 )
		{
			// oversatiation
			messagePlayer(player, language[634]);
			this->char_gonnavomit = 140 + rand() % 60;
		}
		else if ( ticks % 60 == 0 && rand() % 200 == 0 && myStats->EFFECTS[EFF_DRUNK] )
		{
			// drunkenness
			messagePlayer(player, language[634]);
			this->char_gonnavomit = 140 + rand() % 60;
		}
	}
	if ( this->char_gonnavomit > 0 )
	{
		this->char_gonnavomit--;
		if ( this->char_gonnavomit == 0 )
		{
			messagePlayer(player, language[635]);
			myStats->EFFECTS[EFF_VOMITING] = true;
			myStats->EFFECTS_TIMERS[EFF_VOMITING] = 50 + rand() % 20;
			serverUpdateEffects(player);
			if ( player == clientnum )
			{
				camera_shakey += 9;
			}
			else if ( player > 0 && multiplayer == SERVER )
			{
				strcpy((char*)net_packet->data, "SHAK");
				net_packet->data[4] = 0; // turns into 0
				net_packet->data[5] = 9;
				net_packet->address.host = net_clients[player - 1].host;
				net_packet->address.port = net_clients[player - 1].port;
				net_packet->len = 6;
				sendPacketSafe(net_sock, -1, net_packet, player - 1);
			}
			playSoundEntity(this, 78, 96);
		}
	}

	// vomiting
	if ( myStats->EFFECTS[EFF_VOMITING] && ticks % 2 == 0 )
	{
		Entity* entity = spawnGib(this);
		entity->sprite = 29;
		entity->flags[SPRITE] = true;
		entity->flags[GENIUS] = true;
		entity->flags[INVISIBLE] = false;
		entity->yaw = this->yaw - 0.1 + (rand() % 20) * 0.01;
		entity->pitch = (rand() % 360) * PI / 180.0;
		entity->roll = (rand() % 360) * PI / 180.0;
		double vel = (rand() % 15) / 10.f;
		entity->vel_x = vel * cos(entity->yaw);
		entity->vel_y = vel * sin(entity->yaw);
		entity->vel_z = -.5;
		myStats->HUNGER -= 40;
		if ( myStats->HUNGER <= 50 )
		{
			myStats->HUNGER = 50;
			myStats->EFFECTS_TIMERS[EFF_VOMITING] = 1;
		}
		serverSpawnGibForClient(entity);
	}

	// healing over time
	int healring = 0;
	if ( myStats->ring != NULL )
	{
		if ( myStats->ring->type == RING_REGENERATION )
		{
			if ( myStats->ring->beatitude >= 0 )
			{
				healring = 1;
			}
			else
			{
				healring = -1;
			}
		}
	}
	if ( myStats->HP < myStats->MAXHP )
	{
		this->char_heal++;
		if ( healring > 0 || svFlags & SV_FLAG_HUNGER )
		{
			if ( (this->char_heal >= HEAL_TIME && !healring) || (this->char_heal >= HEAL_TIME * 4 && healring < 0) || (this->char_heal >= HEAL_TIME / 8 && healring > 0) )
			{
				this->char_heal = 0;
				this->modHP(1);
			}
		}
	}
	else
	{
		this->char_heal = 0;
	}

	// random teleportation
	if ( myStats->ring != NULL )
	{
		if ( myStats->ring->type == RING_TELEPORTATION )
		{
			if ( rand() % 1000 == 0 )   // .1% chance every frame
			{
				teleportRandom();
			}
		}
	}

	// regaining energy over time
	if ( myStats->MP < myStats->MAXMP )
	{
		this->char_energize++;
		if ( this->char_energize >= MAGIC_REGEN_TIME )
		{
			this->char_energize = 0;
			this->modMP(1);
		}
	}
	else
	{
		this->char_energize = 0;
	}

	// effects of greasy fingers
	if ( myStats->EFFECTS[EFF_GREASY] == true )
	{
		if ( myStats->weapon != NULL )
		{
			messagePlayer(player, language[636]);
			if ( player >= 0 )
			{
				dropItem(myStats->weapon, player);
				if ( player > 0 && multiplayer == SERVER )
				{
					strcpy((char*)net_packet->data, "DROP");
					net_packet->data[4] = 5;
					net_packet->address.host = net_clients[player - 1].host;
					net_packet->address.port = net_clients[player - 1].port;
					net_packet->len = 5;
					sendPacketSafe(net_sock, -1, net_packet, player - 1);
				}
			}
			else
			{
				dropItemMonster(myStats->weapon, this, myStats);
			}
			myStats->weapon = NULL;
		}
	}

	// torches/lamps burn down
	if ( myStats->shield != NULL )
	{
		if ( myStats->shield->type == TOOL_TORCH || myStats->shield->type == TOOL_LANTERN )
		{
			this->char_torchtime++;
			if ( (this->char_torchtime >= 7200 && myStats->shield->type == TOOL_TORCH) || this->char_torchtime >= 10260 )
			{
				this->char_torchtime = 0;
				if ( player == clientnum )
				{
					if ( myStats->shield->count > 1 )
					{
						newItem(myStats->shield->type, myStats->shield->status, myStats->shield->beatitude, myStats->shield->count - 1, myStats->shield->appearance, myStats->shield->identified, &myStats->inventory);
					}
				}
				myStats->shield->count = 1;
				myStats->shield->status = static_cast<Status>(myStats->shield->status - 1);
				if ( myStats->shield->status > BROKEN )
				{
					messagePlayer(player, language[637], myStats->shield->getName());
				}
				else
				{
					messagePlayer(player, language[638], myStats->shield->getName());
				}
				if ( multiplayer == SERVER && player > 0 )
				{
					strcpy((char*)net_packet->data, "ARMR");
					net_packet->data[4] = 4;
					net_packet->data[5] = myStats->shield->status;
					net_packet->address.host = net_clients[player - 1].host;
					net_packet->address.port = net_clients[player - 1].port;
					net_packet->len = 6;
					sendPacketSafe(net_sock, -1, net_packet, player - 1);
				}
			}
		}
	}

	// effects of being poisoned
	if ( myStats->EFFECTS[EFF_POISONED] )
	{
		if ( myStats->amulet != NULL )
		{
			if ( myStats->amulet->type == AMULET_POISONRESISTANCE )
			{
				messagePlayer(player, language[639]);
				messagePlayer(player, language[640]);
				myStats->EFFECTS_TIMERS[EFF_POISONED] = 0;
				myStats->EFFECTS[EFF_POISONED] = false;
				serverUpdateEffects(player);
				this->char_poison = 0;
			}
		}
		this->char_poison++;
		if ( this->char_poison > 180 )   // three seconds
		{
			this->char_poison = 0;
			int poisonhurt = std::max(1 + rand() % 4 - myStats->CON, 3);
			this->modHP(-poisonhurt);
			if ( myStats->HP <= 0 )
			{
				Entity* killer = uidToEntity( myStats->poisonKiller );
				if ( killer )
				{
					killer->awardXP( this, true, true );
				}
			}
			this->setObituary(language[1531]);
			playSoundEntity(this, 28, 64);
			if ( player == clientnum )
			{
				camera_shakex += .1;
				camera_shakey += 10;
			}
			else if ( player > 0 && multiplayer == SERVER )
			{
				strcpy((char*)net_packet->data, "SHAK");
				net_packet->data[4] = 10; // turns into .1
				net_packet->data[5] = 10;
				net_packet->address.host = net_clients[player - 1].host;
				net_packet->address.port = net_clients[player - 1].port;
				net_packet->len = 6;
				sendPacketSafe(net_sock, -1, net_packet, player - 1);
			}
			if ( rand() % 5 == 0 )
			{
				messagePlayer(player, language[641]);
				myStats->EFFECTS_TIMERS[EFF_POISONED] = 0;
				myStats->EFFECTS[EFF_POISONED] = false;
				serverUpdateEffects(player);
			}
		}
	}
	else
	{
		this->char_poison = 0;
	}

	// bleeding
	if ( myStats->EFFECTS[EFF_BLEEDING] )
	{
		if ( ticks % 120 == 0 )
		{
			if ( myStats->HP > 5 )
			{
				int bleedhurt = 1;
				this->modHP(-bleedhurt);
				this->setObituary(language[1532]);
				Entity* gib = spawnGib(this);
				serverSpawnGibForClient(gib);
				if ( player == clientnum )
				{
					camera_shakex -= .03;
					camera_shakey += 3;
				}
				else if ( player > 0 && multiplayer == SERVER )
				{
					strcpy((char*)net_packet->data, "SHAK");
					net_packet->data[4] = -3; // turns into -.03
					net_packet->data[5] = 3;
					net_packet->address.host = net_clients[player - 1].host;
					net_packet->address.port = net_clients[player - 1].port;
					net_packet->len = 6;
					sendPacketSafe(net_sock, -1, net_packet, player - 1);
				}
				messagePlayer(player, language[642]);
				if ( spawn_blood )
				{
					Entity* entity = NULL;
					if ( gibtype[myStats->type] == 1 )
					{
						entity = newEntity(203, 1, map.entities);
					}
					else if ( gibtype[myStats->type] == 2 )
					{
						entity = newEntity(213, 1, map.entities);
					}
					if ( entity != NULL )
					{
						entity->x = this->x;
						entity->y = this->y;
						entity->z = 7.4 + (rand() % 20) / 100.f;
						entity->parent = this->uid;
						entity->sizex = 2;
						entity->sizey = 2;
						entity->yaw = (rand() % 360) * PI / 180.0;
						entity->flags[UPDATENEEDED] = true;
						entity->flags[PASSABLE] = true;
					}
				}
			}
			else
			{
				messagePlayer(player, language[643]);
				myStats->EFFECTS[EFF_BLEEDING] = false;
				myStats->EFFECTS_TIMERS[EFF_BLEEDING] = 0;
				serverUpdateEffects(player);
			}
		}
	}

	// burning
	if ( this->flags[BURNING] )
	{
		if ( ticks % 30 == 0 )
		{
			this->modHP(-2 - rand() % 3);
			if ( myStats->HP <= 0 )
			{
				Entity* killer = uidToEntity( myStats->poisonKiller );
				if ( killer )
				{
					killer->awardXP( this, true, true );
				}
			}
			this->setObituary(language[1533]);
			messagePlayer(player, language[644]);
			playSoundEntity(this, 28, 64);
			if ( player == clientnum )
			{
				camera_shakey += 3;
			}
			else if ( player > 0 && multiplayer == SERVER )
			{
				strcpy((char*)net_packet->data, "SHAK");
				net_packet->data[4] = 0; // turns into 0
				net_packet->data[5] = 3;
				net_packet->address.host = net_clients[player - 1].host;
				net_packet->address.port = net_clients[player - 1].port;
				net_packet->len = 6;
				sendPacketSafe(net_sock, -1, net_packet, player - 1);
			}
			if ( rand() % 10 == 0 )
			{
				if ( myStats->cloak != NULL )
				{
					if ( player == clientnum )
					{
						if ( myStats->cloak->count > 1 )
						{
							newItem(myStats->cloak->type, myStats->cloak->status, myStats->cloak->beatitude, myStats->cloak->count - 1, myStats->cloak->appearance, myStats->cloak->identified, &myStats->inventory);
						}
					}
					myStats->cloak->count = 1;
					myStats->cloak->status = static_cast<Status>(myStats->cloak->status - 1);
					if ( myStats->cloak->status != BROKEN )
					{
						messagePlayer(player, language[645], myStats->cloak->getName());
					}
					else
					{
						messagePlayer(player, language[646], myStats->cloak->getName());
					}
					if ( player > 0 && multiplayer == SERVER )
					{
						strcpy((char*)net_packet->data, "ARMR");
						net_packet->data[4] = 6;
						net_packet->data[5] = myStats->cloak->status;
						net_packet->address.host = net_clients[player - 1].host;
						net_packet->address.port = net_clients[player - 1].port;
						net_packet->len = 6;
						sendPacketSafe(net_sock, -1, net_packet, player - 1);
					}
				}
			}
			if ( rand() % 10 == 0 )
			{
				this->flags[BURNING] = false;
				messagePlayer(player, language[647]);
				if ( player > 0 && multiplayer == SERVER )
				{
					serverUpdateEntityFlag(this, BURNING);
				}
			}
		}
	}

	// amulet effects
	if ( myStats->amulet != NULL )
	{
		// strangulation
		if ( myStats->amulet->type == AMULET_STRANGULATION )
		{
			if ( ticks % 60 == 0 )
			{
				if ( rand() % 25 )
				{
					messagePlayer(player, language[648]);
					this->modHP(-(2 + rand() % 3));
					this->setObituary(language[1534]);
					if ( myStats->HP <= 0 )
					{
						if ( player <= 0 )
						{
							Item* item = myStats->amulet;
							if ( item->count > 1 )
							{
								newItem(item->type, item->status, item->beatitude, item->count - 1, item->appearance, item->identified, &myStats->inventory);
							}
						}
						myStats->amulet->count = 1;
						myStats->amulet->status = BROKEN;
						playSoundEntity(this, 76, 64);
						if ( player > 0 && multiplayer == SERVER )
						{
							strcpy((char*)net_packet->data, "ARMR");
							net_packet->data[4] = 7;
							net_packet->data[5] = myStats->amulet->status;
							net_packet->address.host = net_clients[player - 1].host;
							net_packet->address.port = net_clients[player - 1].port;
							net_packet->len = 6;
							sendPacketSafe(net_sock, -1, net_packet, player - 1);
						}
					}
					if ( player == clientnum )
					{
						camera_shakey += 8;
					}
					else if ( player > 0 && multiplayer == SERVER )
					{
						strcpy((char*)net_packet->data, "SHAK");
						net_packet->data[4] = 0; // turns into 0
						net_packet->data[5] = 8;
						net_packet->address.host = net_clients[player - 1].host;
						net_packet->address.port = net_clients[player - 1].port;
						net_packet->len = 6;
						sendPacketSafe(net_sock, -1, net_packet, player - 1);
					}
				}
				else
				{
					messagePlayer(player, language[649]);
					messagePlayer(player, language[650]);
					if ( player <= 0 )
					{
						Item* item = myStats->amulet;
						if ( item->count > 1 )
						{
							newItem(item->type, item->status, item->beatitude, item->count - 1, item->appearance, item->identified, &myStats->inventory);
						}
					}
					myStats->amulet->count = 1;
					myStats->amulet->status = BROKEN;
					playSoundEntity(this, 76, 64);
					if ( player > 0 && multiplayer == SERVER )
					{
						strcpy((char*)net_packet->data, "ARMR");
						net_packet->data[4] = 7;
						net_packet->data[5] = myStats->amulet->status;
						net_packet->address.host = net_clients[player - 1].host;
						net_packet->address.port = net_clients[player - 1].port;
						net_packet->len = 6;
						sendPacketSafe(net_sock, -1, net_packet, player - 1);
					}
				}
			}
		}
		// life saving
		if ( myStats->amulet->type == AMULET_LIFESAVING )   //TODO: Doesn't save against boulder traps.
		{
			if ( myStats->HP <= 0 )
			{
				if ( myStats->HUNGER > 0 )
				{
					messagePlayer(player, language[651]);
				}
				if ( !this->isBlind())
				{
					messagePlayer(player, language[652]);
				}
				else
				{
					messagePlayer(player, language[653]);
				}
				if ( myStats->amulet->beatitude >= 0 )
				{
					messagePlayer(player, language[654]);
					messagePlayer(player, language[655]);
					steamAchievementClient(player, "BARONY_ACH_BORN_AGAIN");
					myStats->HUNGER = 800;
					if ( myStats->MAXHP < 10 )
					{
						myStats->MAXHP = 10;
						if ( player > 0 && multiplayer == SERVER )
						{
							strcpy((char*)net_packet->data, "ATTR");
							net_packet->data[4] = clientnum;
							net_packet->data[5] = (Sint8)myStats->STR;
							net_packet->data[6] = (Sint8)myStats->DEX;
							net_packet->data[7] = (Sint8)myStats->CON;
							net_packet->data[8] = (Sint8)myStats->INT;
							net_packet->data[9] = (Sint8)myStats->PER;
							net_packet->data[10] = (Sint8)myStats->CHR;
							net_packet->data[11] = (Sint8)myStats->EXP;
							net_packet->data[12] = (Sint8)myStats->LVL;
							SDLNet_Write16((Sint16)myStats->HP, &net_packet->data[13]);
							SDLNet_Write16((Sint16)myStats->MAXHP, &net_packet->data[15]);
							SDLNet_Write16((Sint16)myStats->MP, &net_packet->data[17]);
							SDLNet_Write16((Sint16)myStats->MAXMP, &net_packet->data[19]);
							net_packet->address.host = net_clients[player - 1].host;
							net_packet->address.port = net_clients[player - 1].port;
							net_packet->len = 21;
							sendPacketSafe(net_sock, -1, net_packet, player - 1);
						}
					}
					this->setHP(std::max(myStats->MAXHP, 10));
					for ( c = 0; c < NUMEFFECTS; c++ )
					{
						myStats->EFFECTS[c] = false;
						myStats->EFFECTS_TIMERS[c] = 0;
					}
					this->flags[BURNING] = false;
					serverUpdateEntityFlag(this, BURNING);
					serverUpdateEffects(player);
				}
				else
				{
					messagePlayer(player, language[656]);
					messagePlayer(player, language[657]);
				}
				myStats->amulet->status = BROKEN;
				playSoundEntity(this, 76, 64);
				if ( player > 0 && multiplayer == SERVER )
				{
					strcpy((char*)net_packet->data, "ARMR");
					net_packet->data[4] = 7;
					net_packet->data[5] = myStats->amulet->status;
					net_packet->address.host = net_clients[player - 1].host;
					net_packet->address.port = net_clients[player - 1].port;
					net_packet->len = 6;
					sendPacketSafe(net_sock, -1, net_packet, player - 1);
				}
				myStats->amulet = NULL;
			}
		}
	}

	// unparalyze certain boss characters
	if ( myStats->EFFECTS[EFF_PARALYZED] && myStats->type >= LICH )
	{
		myStats->EFFECTS[EFF_PARALYZED] = false;
		myStats->EFFECTS_TIMERS[EFF_PARALYZED] = 0;
	}

	// wake up
	if ( myStats->EFFECTS[EFF_ASLEEP] && (myStats->OLDHP != myStats->HP || myStats->type >= LICH) )
	{
		messagePlayer(player, language[658]);
		myStats->EFFECTS[EFF_ASLEEP] = false;
		myStats->EFFECTS_TIMERS[EFF_ASLEEP] = 0;
		serverUpdateEffects(player);
	}
	myStats->OLDHP = myStats->HP;
}

/*-------------------------------------------------------------------------------

	Entity::getAttack

	returns the attack power of an entity based on strength, weapon, and a
	base number

-------------------------------------------------------------------------------*/

Sint32 Entity::getAttack()
{
	Stat* entitystats;
	Sint32 attack = 0;

	if ( (entitystats = this->getStats()) == NULL )
	{
		return 0;
	}

	attack = 8; // base attack strength
	if ( entitystats->weapon != NULL)
	{
		attack += entitystats->weapon->weaponGetAttack();
	}
	attack += this->getSTR();

	return attack;
}

/*-------------------------------------------------------------------------------

	Entity::getSTR()

	returns the STR attribute of an entity, post modifiers

-------------------------------------------------------------------------------*/

Sint32 Entity::getSTR()
{
	Stat* entitystats;

	if ( (entitystats = this->getStats()) == NULL )
	{
		return 0;
	}
	return statGetSTR(entitystats);
}

Sint32 statGetSTR(Stat* entitystats)
{
	Sint32 STR;

	STR = entitystats->STR;
	if ( entitystats->HUNGER >= 1500 )
	{
		STR--;
	}
	if ( entitystats->HUNGER <= 150 )
	{
		STR--;
	}
	if ( entitystats->HUNGER <= 50 )
	{
		STR--;
	}
	if ( entitystats->gloves != NULL )
		if ( entitystats->gloves->type == GAUNTLETS_STRENGTH )
		{
			STR++;
		}
	if ( entitystats->ring != NULL )
	{
		if ( entitystats->ring->type == RING_STRENGTH )
		{
			if ( entitystats->ring->beatitude >= 0 )
			{
				STR++;
			}
			else
			{
				STR--;
			}
		}
	}
	if ( entitystats->EFFECTS[EFF_DRUNK] )
	{
		STR++;
	}
	return STR;
}

/*-------------------------------------------------------------------------------

	Entity::getDEX

	returns the DEX attribute of an entity, post modifiers

-------------------------------------------------------------------------------*/

Sint32 Entity::getDEX()
{
	Stat* entitystats;

	if ( (entitystats = this->getStats()) == NULL )
	{
		return 0;
	}
	return statGetDEX(entitystats);
}

Sint32 statGetDEX(Stat* entitystats)
{
	Sint32 DEX;

	// paralyzed
	if ( entitystats->EFFECTS[EFF_PARALYZED] )
	{
		return -10;
	}
	if ( entitystats->EFFECTS[EFF_ASLEEP] )
	{
		return -10;
	}

	DEX = entitystats->DEX;
	if ( entitystats->EFFECTS[EFF_FAST] && !entitystats->EFFECTS[EFF_SLOW] )
	{
		DEX += 10;
	}
	if ( entitystats->EFFECTS[EFF_STUNNED] )
	{
		DEX -= 5;
	}
	if ( entitystats->HUNGER >= 1500 )
	{
		DEX--;
	}
	if ( entitystats->HUNGER <= 150 )
	{
		DEX--;
	}
	if ( entitystats->HUNGER <= 50 )
	{
		DEX--;
	}
	if ( !entitystats->EFFECTS[EFF_FAST] && entitystats->EFFECTS[EFF_SLOW] )
	{
		DEX = std::min(DEX - 3, -2);
	}
	if ( entitystats->shoes != NULL )
		if ( entitystats->shoes->type == LEATHER_BOOTS_SPEED )
		{
			DEX++;
		}
	if ( entitystats->gloves != NULL )
		if ( entitystats->gloves->type == GLOVES_DEXTERITY )
		{
			DEX++;
		}
	if ( entitystats->EFFECTS[EFF_DRUNK] )
	{
		DEX--;
	}
	return DEX;
}

/*-------------------------------------------------------------------------------

	Entity::getCON

	returns the CON attribute of an entity, post modifiers

-------------------------------------------------------------------------------*/

Sint32 Entity::getCON()
{
	Stat* entitystats;

	if ( (entitystats = this->getStats()) == NULL )
	{
		return 0;
	}
	return statGetCON(entitystats);
}

Sint32 statGetCON(Stat* entitystats)
{
	Sint32 CON;

	CON = entitystats->CON;
	if ( entitystats->ring != NULL )
	{
		if ( entitystats->ring->type == RING_CONSTITUTION )
		{
			if ( entitystats->ring->beatitude >= 0 )
			{
				CON++;
			}
			else
			{
				CON--;
			}
		}
	}
	if ( entitystats->gloves != NULL )
	{
		if ( entitystats->gloves->type == BRACERS_CONSTITUTION )
		{
			if ( entitystats->gloves->beatitude >= 0 )
			{
				CON++;
			}
			else
			{
				CON--;
			}
		}
	}
	return CON;
}

/*-------------------------------------------------------------------------------

	Entity::getINT

	returns the INT attribute of an entity, post modifiers

-------------------------------------------------------------------------------*/

Sint32 Entity::getINT()
{
	Stat* entitystats;

	if ( (entitystats = this->getStats()) == NULL )
	{
		return 0;
	}
	return statGetINT(entitystats);
}

Sint32 statGetINT(Stat* entitystats)
{
	Sint32 INT;

	INT = entitystats->INT;
	if ( entitystats->HUNGER <= 50 )
	{
		INT--;
	}
	if ( entitystats->helmet != NULL )
		if ( entitystats->helmet->type == HAT_WIZARD )
		{
			INT++;
		}
	return INT;
}

/*-------------------------------------------------------------------------------

	Entity::getPER

	returns the PER attribute of an entity, post modifiers

-------------------------------------------------------------------------------*/

Sint32 Entity::getPER()
{
	Stat* entitystats;

	if ( (entitystats = this->getStats()) == NULL )
	{
		return 0;
	}
	return statGetPER(entitystats);
}

Sint32 statGetPER(Stat* entitystats)
{
	Sint32 PER;

	PER = entitystats->PER;
	if ( entitystats->HUNGER <= 50 )
	{
		PER--;
	}
	if ( entitystats->mask )
		if ( entitystats->mask->type == TOOL_GLASSES )
		{
			PER++;
		}
	return PER;
}

/*-------------------------------------------------------------------------------

	Entity::getCHR

	returns the CHR attribute of an entity, post modifiers

-------------------------------------------------------------------------------*/

Sint32 Entity::getCHR()
{
	Stat* entitystats;

	if ( (entitystats = this->getStats()) == NULL )
	{
		return 0;
	}
	return statGetCHR(entitystats);
}

Sint32 statGetCHR(Stat* entitystats)
{
	Sint32 CHR;

	CHR = entitystats->CHR;
	if ( entitystats->helmet != NULL )
		if ( entitystats->helmet->type == HAT_JESTER )
		{
			CHR++;
		}
	if ( entitystats->ring != NULL )
		if ( entitystats->ring->type == RING_ADORNMENT )
		{
			if ( entitystats->ring->beatitude >= 0 )
			{
				CHR++;
			}
			else
			{
				CHR--;
			}
		}
	return CHR;
}

/*-------------------------------------------------------------------------------

	Entity::isBlind

	returns true if the given entity is blind, and false if it is not

-------------------------------------------------------------------------------*/

bool Entity::isBlind()
{
	Stat* entitystats;
	if ( (entitystats = this->getStats()) == NULL )
	{
		return false;
	}

	// being blind
	if ( entitystats->EFFECTS[EFF_BLIND] == true )
	{
		return true;
	}

	// asleep
	if ( entitystats->EFFECTS[EFF_ASLEEP] == true )
	{
		return true;
	}

	// messy face
	if ( entitystats->EFFECTS[EFF_MESSY] == true )
	{
		return true;
	}

	// wearing blindfolds
	if ( entitystats->mask != NULL )
		if ( entitystats->mask->type == TOOL_BLINDFOLD )
		{
			return true;
		}

	return false;
}

/*-------------------------------------------------------------------------------

	Entity::isInvisible

	returns true if the given entity is invisible or else wearing something
	that would make it invisible

-------------------------------------------------------------------------------*/

bool Entity::isInvisible()
{
	Stat* entitystats;
	if ( (entitystats = getStats()) == NULL )
	{
		return false;
	}

	// being invisible
	if ( entitystats->EFFECTS[EFF_INVISIBLE] == true )
	{
		return true;
	}

	// wearing invisibility cloaks
	if ( entitystats->cloak != NULL )
		if ( entitystats->cloak->type == CLOAK_INVISIBILITY )
		{
			return true;
		}

	// wearing invisibility ring
	if ( entitystats->ring != NULL )
		if ( entitystats->ring->type == RING_INVISIBILITY )
		{
			return true;
		}

	return false;
}

/*-------------------------------------------------------------------------------

	Entity::isMobile

	returns true if the given entity can move, or false if it cannot

-------------------------------------------------------------------------------*/

bool Entity::isMobile()
{
	Stat* entitystats;
	if ( (entitystats = getStats()) == NULL )
	{
		return true;
	}

	// paralyzed
	if ( entitystats->EFFECTS[EFF_PARALYZED] )
	{
		return false;
	}

	// asleep
	if ( entitystats->EFFECTS[EFF_ASLEEP] )
	{
		return false;
	}

	return true;
}

/*-------------------------------------------------------------------------------

	checkTileForEntity

	returns a list of entities that are occupying the map tile specified at
	(x, y)

-------------------------------------------------------------------------------*/

list_t* checkTileForEntity(int x, int y)
{
	list_t* return_val = NULL;

	//Loop through the list.
	//If the entity's x and y match the tile's x and y (correcting for the difference in the two x/y systems, of course), then the entity is on the tile.
	//Traverse map.entities...
	node_t* node = NULL;
	node_t* node2 = NULL;
	#ifdef __ARM_NEON__
	const int32x2_t xy = {x, y};
	#endif
	
	for ( node = map.entities->first; node != NULL; node = node->next )
	{
		if (node->element)
		{
			Entity* entity = (Entity*)node->element;
			if (entity) {
			#ifdef __ARM_NEON__
			uint32x2_t eqxy =vceq_s32(vcvt_s32_f32(vmul_n_f32(vld1_f32(&entity->x), 1.0f/16.0f)), xy);
			if ( eqxy[0] && eqxy[1] )
			#else
			if ( (int)floor((entity->x / 16)) == x && (int)floor((entity->y / 16)) == y)   //Check if the current entity is on the tile.
			#endif
			{
				//Right. So. Create the list if it doesn't exist.
				if (!return_val)
				{
					return_val = (list_t*) malloc(sizeof(list_t));
					return_val->first = NULL;
					return_val->last = NULL;
				}

				//And add the current entity to it.
				node2 = list_AddNodeLast(return_val);
				node2->element = entity;
				node2->deconstructor = &emptyDeconstructor;
			}
			}
		}
	}

	return return_val;
}

/*-------------------------------------------------------------------------------

	getItemsOnTile

	Fills the given list with nodes for every item entity on the given
	map tile (x, y)

-------------------------------------------------------------------------------*/

void getItemsOnTile(int x, int y, list_t** list)
{

	//Take the return value of checkTileForEntity() and sort that list for items.
	//if( entity->behavior == &actItem )
	//And then free the list returned by checkTileForEntity.

	//Right. First, grab all the entities on the tile.
	list_t* entities = NULL;
	entities = checkTileForEntity(x, y);

	if (!entities)
	{
		return;    //No use continuing of got no entities.
	}

	node_t* node = NULL;
	node_t* node2 = NULL;
	//Loop through the list of entities.
	for (node = entities->first; node != NULL; node = node->next)
	{
		if (node->element)
		{
			Entity* entity = (Entity*) node->element;
			//Check if the entity is an item.
			if (entity && entity->behavior == &actItem)
			{
				//If this is the first item found, the list needs to be created.
				if (!(*list))
				{
					*list = (list_t*) malloc(sizeof(list_t));
					(*list)->first = NULL;
					(*list)->last = NULL;
				}

				//Add the current entity to it.
				node2 = list_AddNodeLast(*list);
				node2->element = entity;
				node2->deconstructor = &emptyDeconstructor;
			}
		}
	}

	if (entities)
	{
		list_FreeAll(entities);
		free(entities);
	}

	//return return_val;
}

/*-------------------------------------------------------------------------------

	Entity::attack

	Causes an entity to attack using whatever weapon it's holding

-------------------------------------------------------------------------------*/

void Entity::attack(int pose, int charge)
{
	Stat* hitstats = NULL;
	Stat* myStats;
	Entity* entity;
	int player, playerhit = -1;
	double dist;
	int c, i;
	int weaponskill = -1;
	node_t* node;
	double tangent;

	if ( (myStats = getStats()) == NULL )
	{
		return;
	}

	// get the player number, if applicable
	if (behavior == &actPlayer)
	{
		player = skill[2];
	}
	else
	{
		player = -1;    // not a player
	}

	if (multiplayer != CLIENT)
	{
		// animation
		if (player >= 0)
		{
			if (stats[player]->weapon != nullptr)
			{
				players[player]->entity->skill[9] = pose;    // PLAYER_ATTACK
			}
			else
			{
				players[player]->entity->skill[9] = 1;    // special case for punch to eliminate spanking motion :p
			}
			players[player]->entity->skill[10] = 0; // PLAYER_ATTACKTIME
		}
		else
		{
			if (myStats->weapon != nullptr)
			{
				monster_attack = pose;
			}
			else
			{
				monster_attack = 1;    // punching
			}
			monster_attacktime = 0;
		}
		if (multiplayer == SERVER)
		{
			if (player >= 0 && player < MAXPLAYERS)
			{
				serverUpdateEntitySkill(players[player]->entity, 9);
				serverUpdateEntitySkill(players[player]->entity, 10);
			}
			else
			{
				serverUpdateEntitySkill(this, 8);
				serverUpdateEntitySkill(this, 9);
			}
		}

		if ( myStats->weapon != NULL )
		{
			// magical weapons
			if ( itemCategory(myStats->weapon) == SPELLBOOK || itemCategory(myStats->weapon) == MAGICSTAFF )
			{
				if ( itemCategory(myStats->weapon) == MAGICSTAFF )
				{
					switch ( myStats->weapon->type )
					{
						case MAGICSTAFF_LIGHT:
							castSpell(uid, &spell_light, true, false);
							break;
						case MAGICSTAFF_DIGGING:
							castSpell(uid, &spell_dig, true, false);
							break;
						case MAGICSTAFF_LOCKING:
							castSpell(uid, &spell_locking, true, false);
							break;
						case MAGICSTAFF_MAGICMISSILE:
							castSpell(uid, &spell_magicmissile, true, false);
							break;
						case MAGICSTAFF_OPENING:
							castSpell(uid, &spell_opening, true, false);
							break;
						case MAGICSTAFF_SLOW:
							castSpell(uid, &spell_slow, true, false);
							break;
						case MAGICSTAFF_COLD:
							castSpell(uid, &spell_cold, true, false);
							break;
						case MAGICSTAFF_FIRE:
							castSpell(uid, &spell_fireball, true, false);
							break;
						case MAGICSTAFF_LIGHTNING:
							castSpell(uid, &spell_lightning, true, false);
							break;
						case MAGICSTAFF_SLEEP:
							castSpell(uid, &spell_sleep, true, false);
							break;
						default:
							messagePlayer(player, "This is my wish stick! Wishy wishy wish!");
							break;
					}

					// magicstaffs deplete themselves for each use
					if ( rand() % 3 == 0 )
					{
						if ( player == clientnum )
						{
							if ( myStats->weapon->count > 1 )
							{
								newItem(myStats->weapon->type, myStats->weapon->status, myStats->weapon->beatitude, myStats->weapon->count - 1, myStats->weapon->appearance, myStats->weapon->identified, &myStats->inventory);
							}
						}
						myStats->weapon->count = 1;
						myStats->weapon->status = static_cast<Status>(myStats->weapon->status - 1);
						if ( myStats->weapon->status != BROKEN )
						{
							messagePlayer(player, language[659]);
						}
						else
						{
							messagePlayer(player, language[660]);
						}
						if ( player > 0 && multiplayer == SERVER )
						{
							strcpy((char*)net_packet->data, "ARMR");
							net_packet->data[4] = 5;
							net_packet->data[5] = myStats->weapon->status;
							net_packet->address.host = net_clients[player - 1].host;
							net_packet->address.port = net_clients[player - 1].port;
							net_packet->len = 6;
							sendPacketSafe(net_sock, -1, net_packet, player - 1);
						}
					}
				}
				else
				{
					// this is mostly used for monsters that "cast" spells
					switch ( myStats->weapon->type )
					{
						case SPELLBOOK_FORCEBOLT:
							castSpell(uid, &spell_forcebolt, true, false);
							break;
						case SPELLBOOK_MAGICMISSILE:
							castSpell(uid, &spell_magicmissile, true, false);
							break;
						case SPELLBOOK_COLD:
							castSpell(uid, &spell_cold, true, false);
							break;
						case SPELLBOOK_FIREBALL:
							castSpell(uid, &spell_fireball, true, false);
							break;
						case SPELLBOOK_LIGHTNING:
							castSpell(uid, &spell_lightning, true, false);
							break;
						case SPELLBOOK_SLEEP:
							castSpell(uid, &spell_sleep, true, false);
							break;
						case SPELLBOOK_CONFUSE:
							castSpell(uid, &spell_confuse, true, false);
							break;
						case SPELLBOOK_SLOW:
							castSpell(uid, &spell_slow, true, false);
							break;
						case SPELLBOOK_DIG:
							castSpell(uid, &spell_dig, true, false);
							break;
						default:
							break;
					}

					// DEPRECATED!!
					/*if( myStats->MP>0 ) {
						castMagic(my);

						// spells deplete MP
						myStats->MP--;
						if( multiplayer==SERVER && player!=clientnum ) {
							strcpy((char *)net_packet->data,"UPMP");
							SDLNet_Write32((Uint32)myStats->MP,&net_packet->data[4]);
							net_packet->address.host = net_clients[player-1].host;
							net_packet->address.port = net_clients[player-1].port;
							net_packet->len = 8;
							sendPacketSafe(net_sock, -1, net_packet, player-1);
						}
					} else {
						messagePlayer(player,"You lack the energy to cast magic!");
					}*/
				}
				return;
			}

			// ranged weapons (bows)
			else if ( myStats->weapon->type == SHORTBOW || myStats->weapon->type == CROSSBOW || myStats->weapon->type == SLING || myStats->weapon->type == ARTIFACT_BOW )
			{
				// damage weapon if applicable
				if ( rand() % 50 == 0 && myStats->weapon->type != ARTIFACT_BOW )
				{
					if ( myStats->weapon != NULL )
					{
						if ( player == clientnum )
						{
							if ( myStats->weapon->count > 1 )
							{
								newItem(myStats->weapon->type, myStats->weapon->status, myStats->weapon->beatitude, myStats->weapon->count - 1, myStats->weapon->appearance, myStats->weapon->identified, &myStats->inventory);
							}
						}
						myStats->weapon->count = 1;
						myStats->weapon->status = static_cast<Status>(myStats->weapon->status - 1);
						if ( myStats->weapon->status != BROKEN )
						{
							messagePlayer(player, language[661], myStats->weapon->getName());
						}
						else
						{
							playSoundEntity(this, 76, 64);
							messagePlayer(player, language[662], myStats->weapon->getName());
						}
						if ( player > 0 && multiplayer == SERVER )
						{
							strcpy((char*)net_packet->data, "ARMR");
							net_packet->data[4] = 5;
							net_packet->data[5] = myStats->weapon->status;
							net_packet->address.host = net_clients[player - 1].host;
							net_packet->address.port = net_clients[player - 1].port;
							net_packet->len = 6;
							sendPacketSafe(net_sock, -1, net_packet, player - 1);
						}
					}
				}
				if ( myStats->weapon->type == SLING )
				{
					entity = newEntity(78, 1, map.entities); // rock
					playSoundEntity(this, 239 + rand() % 3, 96);
				}
				else if ( myStats->weapon->type == CROSSBOW )
				{
					entity = newEntity(167, 1, map.entities); // bolt
					playSoundEntity(this, 239 + rand() % 3, 96);
				}
				else
				{
					entity = newEntity(166, 1, map.entities); // arrow
					playSoundEntity(this, 239 + rand() % 3, 96);
				}
				entity->parent = uid;
				entity->x = x;
				entity->y = y;
				entity->z = z;
				entity->yaw = yaw;
				entity->sizex = 1;
				entity->sizey = 1;
				entity->behavior = &actArrow;
				entity->flags[UPDATENEEDED] = true;
				entity->flags[PASSABLE] = true;

				// arrow power
				entity->skill[3] = getAttack() - 1 + myStats->PROFICIENCIES[PRO_RANGED] / 20;

				// poison arrow
				if ( myStats->weapon->type == ARTIFACT_BOW )
				{
					entity->skill[4] = 540;    // 9 seconds of poison
				}
				return;
			}

			// potions and gems (throwing)
			if ( itemCategory(myStats->weapon) == POTION || itemCategory(myStats->weapon) == GEM )
			{
				playSoundEntity(this, 75, 64);
				entity = newEntity(itemModel(myStats->weapon), 1, map.entities); // thrown item
				entity->parent = uid;
				entity->x = x;
				entity->y = y;
				entity->z = z;
				entity->yaw = yaw;
				entity->sizex = 1;
				entity->sizey = 1;
				entity->behavior = &actThrown;
				entity->flags[UPDATENEEDED] = true;
				entity->flags[PASSABLE] = true;
				entity->skill[10] = myStats->weapon->type;
				entity->skill[11] = myStats->weapon->status;
				entity->skill[12] = myStats->weapon->beatitude;
				entity->skill[13] = 1;
				entity->skill[14] = myStats->weapon->appearance;
				entity->skill[15] = myStats->weapon->identified;
				entity->vel_x = 5 * cos(players[player]->entity->yaw);
				entity->vel_y = 5 * sin(players[player]->entity->yaw);
				entity->vel_z = -.5;

				myStats->weapon->count--;
				if ( myStats->weapon->count <= 0 )
				{
					if ( myStats->weapon->node )
					{
						list_RemoveNode(myStats->weapon->node);
					}
					else
					{
						free(myStats->weapon);
					}
					myStats->weapon = NULL;
				}
				return;
			}
		}

		// normal attacks
		playSoundEntity(this, 23 + rand() % 5, 128); // whoosh noise
		dist = lineTrace(this, x, y, yaw, STRIKERANGE, 0, false);
		if ( hit.entity != NULL )
		{
			if ( !(svFlags & SV_FLAG_FRIENDLYFIRE) )
			{
				// test for friendly fire
				if ( checkFriend(hit.entity) )
				{
					return;
				}
			}

			if ( hit.entity->behavior == &actBoulder )
			{
				if ( myStats->weapon != NULL )
				{
					if ( myStats->weapon->type == TOOL_PICKAXE )
					{
						// spawn several rock items
						int i = 8 + rand() % 4;

						int c;
						for ( c = 0; c < i; c++ )
						{
							Entity* entity = newEntity(-1, 1, map.entities);
							entity->flags[INVISIBLE] = true;
							entity->flags[UPDATENEEDED] = true;
							entity->x = hit.entity->x - 4 + rand() % 8;
							entity->y = hit.entity->y - 4 + rand() % 8;
							entity->z = -6 + rand() % 12;
							entity->sizex = 4;
							entity->sizey = 4;
							entity->yaw = rand() % 360 * PI / 180;
							entity->vel_x = (rand() % 20 - 10) / 10.0;
							entity->vel_y = (rand() % 20 - 10) / 10.0;
							entity->vel_z = -.25 - (rand() % 5) / 10.0;
							entity->flags[PASSABLE] = true;
							entity->behavior = &actItem;
							entity->flags[USERFLAG1] = true; // no collision: helps performance
							entity->skill[10] = GEM_ROCK;    // type
							entity->skill[11] = WORN;        // status
							entity->skill[12] = 0;           // beatitude
							entity->skill[13] = 1;           // count
							entity->skill[14] = 0;           // appearance
							entity->skill[15] = false;       // identified
						}

						double ox = hit.entity->x;
						double oy = hit.entity->y;

						// destroy the boulder
						playSoundEntity(hit.entity, 67, 128);
						list_RemoveNode(hit.entity->mynode);
						messagePlayer(player, language[663]);
						if ( rand() % 2 )
						{
							myStats->weapon->status = static_cast<Status>(myStats->weapon->status - 1);
							if ( myStats->weapon->status == BROKEN )
							{
								messagePlayer(player, language[664]);
								playSoundEntity(this, 76, 64);
							}
							else
							{
								messagePlayer(player, language[665]);
							}
							if ( player > 0 && multiplayer == SERVER )
							{
								strcpy((char*)net_packet->data, "ARMR");
								net_packet->data[4] = 5;
								net_packet->data[5] = myStats->weapon->status;
								net_packet->address.host = net_clients[player - 1].host;
								net_packet->address.port = net_clients[player - 1].port;
								net_packet->len = 6;
								sendPacketSafe(net_sock, -1, net_packet, player - 1);
							}
						}

						// on sokoban, destroying boulders spawns scorpions
						if ( !strcmp(map.name, "Sokoban") )
						{
							Entity* monster = summonMonster(SCORPION, ox, oy);
							if ( monster )
							{
								int c;
								for ( c = 0; c < MAXPLAYERS; c++ )
								{
									Uint32 color = SDL_MapRGB(mainsurface->format, 255, 128, 0);
									messagePlayerColor(c, color, language[406]);
								}
							}
						}
					}
					else
					{
						spawnBang(hit.x - cos(yaw) * 2, hit.y - sin(yaw) * 2, 0);
					}
				}
				else
				{
					//spawnBang(hit.x - cos(my->yaw)*2,hit.y - sin(my->yaw)*2,0);
					playSoundPos(hit.x, hit.y, 183, 64);
				}
			}
			else if ( hit.entity->behavior == &actMonster )
			{
				if ( hit.entity->children.first != NULL )
				{
					if ( hit.entity->children.first->next != NULL )
					{
						hitstats = (Stat*)hit.entity->children.first->next->element;

						// alert the monster!
						if ( hit.entity->skill[0] != 1 && (hitstats->type < LICH || hitstats->type >= SHOPKEEPER) )
						{
							//hit.entity->skill[0]=0;
							//hit.entity->skill[4]=0;
							//hit.entity->fskill[4]=atan2(my->y-hit.entity->y,my->x-hit.entity->x);
							hit.entity->skill[0] = 2;
							hit.entity->skill[1] = uid;
							hit.entity->fskill[2] = x;
							hit.entity->fskill[3] = y;
						}

						// alert other monsters too
						Entity* ohitentity = hit.entity;
						for ( node = map.entities->first; node != NULL; node = node->next )
						{
							entity = (Entity*)node->element;
							if ( entity && entity->behavior == &actMonster && entity != ohitentity )
							{
								Stat* buddystats = entity->getStats();
								if ( buddystats != NULL )
								{
									if ( entity->checkFriend(hit.entity) )
									{
										if ( entity->skill[0] == 0 )   // monster is waiting
										{
											tangent = atan2( entity->y - ohitentity->y, entity->x - ohitentity->x );
											lineTrace(ohitentity, ohitentity->x, ohitentity->y, tangent, 1024, 0, false);
											if ( hit.entity == entity )
											{
												entity->skill[0] = 2; // path state
												entity->skill[1] = uid;
												entity->fskill[2] = x;
												entity->fskill[3] = y;
											}
										}
									}
								}
							}
						}
						hit.entity = ohitentity;
					}
				}
			}
			else if ( hit.entity->behavior == &actPlayer )
			{
				hitstats = stats[hit.entity->skill[2]];
				playerhit = hit.entity->skill[2];

				// alert the player's followers!
				for ( node = hitstats->FOLLOWERS.first; node != NULL; node = node->next )
				{
					Uint32* c = (Uint32*)node->element;
					entity = uidToEntity(*c);
					Entity* ohitentity = hit.entity;
					if ( entity )
					{
						Stat* buddystats = entity->getStats();
						if ( buddystats != NULL )
						{
							if ( entity->skill[0] == 0 || (entity->skill[0] == 3 && entity->skill[1] != uid) )   // monster is waiting or hunting
							{
								entity->skill[0] = 2; // path state
								entity->skill[1] = uid;
								entity->fskill[2] = x;
								entity->fskill[3] = y;
							}
						}
					}
					hit.entity = ohitentity;
				}
			}
			else if ( hit.entity->behavior == &actDoor || hit.entity->behavior == &actFurniture || hit.entity->behavior == &actChest )
			{
				int axe = 0;
				if ( myStats->weapon )
				{
					if ( myStats->weapon->type == BRONZE_AXE || myStats->weapon->type == IRON_AXE || myStats->weapon->type == STEEL_AXE )
					{
						axe = 1; // axes do extra damage to doors :)
					}
				}
				if ( hit.entity->behavior != &actChest )
				{
					if ( charge < MAXCHARGE / 2 )
					{
						hit.entity->skill[4] -= 1 + axe; // decrease door/furniture health
					}
					else
					{
						hit.entity->skill[4] -= 2 + axe; // decrease door/furniture health extra
					}
				}
				else
				{
					if ( charge < MAXCHARGE / 2 )
					{
						hit.entity->skill[3] -= 1 + axe; // decrease chest health
					}
					else
					{
						hit.entity->skill[3] -= 2 + axe; // decrease chest health extra
					}
				}
				playSoundEntity(hit.entity, 28, 64);
				if ( (hit.entity->behavior != &actChest && hit.entity->skill[4] > 0) || (hit.entity->behavior == &actChest && hit.entity->skill[3] > 0) )
				{
					if ( hit.entity->behavior == &actDoor )
					{
						messagePlayer(player, language[666]);
					}
					else if ( hit.entity->behavior == &actChest )
					{
						messagePlayer(player, language[667]);
					}
					else if ( hit.entity->behavior == &actFurniture )
					{
						if ( hit.entity->skill[0] == 0 )
						{
							messagePlayer(player, language[668]);
						}
						else
						{
							messagePlayer(player, language[669]);
						}
					}
				}
				else
				{
					hit.entity->skill[4] = 0;
					if ( hit.entity->behavior == &actDoor )
					{
						messagePlayer(player, language[670]);
						if ( !hit.entity->skill[0] )
						{
							hit.entity->skill[6] = (x > hit.entity->x);
						}
						else
						{
							hit.entity->skill[6] = (y < hit.entity->y);
						}
					}
					else if ( hit.entity->behavior == &actChest )
					{
						messagePlayer(player, language[671]);
					}
					else if ( hit.entity->behavior == &actFurniture )
					{
						if ( hit.entity->skill[0] == 0 )
						{
							messagePlayer(player, language[672]);
						}
						else
						{
							messagePlayer(player, language[673]);
						}
					}
				}
				if ( hit.entity->behavior == &actDoor )
				{
					updateEnemyBar(this, hit.entity, language[674], hit.entity->skill[4], hit.entity->skill[9]);
				}
				else if ( hit.entity->behavior == &actChest )
				{
					updateEnemyBar(this, hit.entity, language[675], hit.entity->skill[3], hit.entity->skill[8]);
				}
				else if ( hit.entity->behavior == &actFurniture )
				{
					if ( hit.entity->skill[0] == 0 )
					{
						updateEnemyBar(this, hit.entity, language[676], hit.entity->skill[4], hit.entity->skill[9]);
					}
					else
					{
						updateEnemyBar(this, hit.entity, language[677], hit.entity->skill[4], hit.entity->skill[9]);
					}
				}
			}
			else if ( hit.entity->behavior == &actSink )
			{
				playSoundEntity(hit.entity, 28, 64);
				playSoundEntity(hit.entity, 140 + rand(), 64);
				messagePlayer(player, language[678]);
				if (hit.entity->skill[0] > 0)
				{
					hit.entity->skill[0]--; //Deplete one usage.

					//50% chance spawn a slime.
					if (rand() % 2 == 0)
					{
						// spawn slime
						Entity* monster = summonMonster(SLIME, x, y);
						if ( monster )
						{
							messagePlayer(player, language[582]);
							Stat* monsterStats = monster->getStats();
							monsterStats->LVL = 4;
						}
					}

					if (hit.entity->skill[0] == 0)   //Depleted.
					{
						messagePlayer(player, language[585]); //TODO: Alert all players that see (or otherwise in range) it?
						playSoundEntity(hit.entity, 132, 64);
					}
				}
			}
			else
			{
				if ( myStats->weapon )
				{
					// bang
					spawnBang(hit.x - cos(yaw) * 2, hit.y - sin(yaw) * 2, 0);
				}
				else
				{
					playSoundPos(hit.x, hit.y, 183, 64);
				}
			}

			if ( hitstats != NULL )
			{
				// hit chance
				//int hitskill=5; // for unarmed combat
				if ( myStats->weapon != NULL )
				{
					if ( myStats->weapon->type == QUARTERSTAFF || myStats->weapon->type == IRON_SPEAR || myStats->weapon->type == STEEL_HALBERD || myStats->weapon->type == ARTIFACT_SPEAR )
					{
						weaponskill = PRO_POLEARM;
					}
					if ( myStats->weapon->type == BRONZE_SWORD || myStats->weapon->type == IRON_SWORD || myStats->weapon->type == STEEL_SWORD || myStats->weapon->type == ARTIFACT_SWORD )
					{
						weaponskill = PRO_SWORD;
					}
					if ( myStats->weapon->type == BRONZE_MACE || myStats->weapon->type == IRON_MACE || myStats->weapon->type == STEEL_MACE || myStats->weapon->type == ARTIFACT_MACE )
					{
						weaponskill = PRO_MACE;
					}
					if ( myStats->weapon->type == BRONZE_AXE || myStats->weapon->type == IRON_AXE || myStats->weapon->type == STEEL_AXE || myStats->weapon->type == ARTIFACT_AXE )
					{
						weaponskill = PRO_AXE;
					}
				}
				/*if( weaponskill>=0 )
					hitskill = myStats->PROFICIENCIES[weaponskill]/5;
				c = rand()%20 + hitskill + (weaponskill==PRO_POLEARM);
				bool hitsuccess=false;
				if( myStats->weapon ) {
					if( myStats->weapon->type == ARTIFACT_SPEAR ) {
						hitsuccess=true; // Gungnir always lands a hit!
					}
				}
				if( c > 10+std::min(std::max(-3,hit.entity->getDEX()-my->getDEX()),3) ) {
					hitsuccess=true;
				}
				if( hitsuccess )*/
				{
					// skill increase
					if ( weaponskill >= 0 )
						if ( rand() % 10 == 0 )
						{
							this->increaseSkill(weaponskill);
						}

					// calculate and perform damage to opponent
					int damage = 0;
					if ( weaponskill >= 0 )
					{
						damage = std::max(0, getAttack() - AC(hitstats)) * damagetables[hitstats->type][weaponskill - PRO_SWORD];
					}
					else
					{
						damage = std::max(0, getAttack() - AC(hitstats));
					}
					if ( weaponskill == PRO_AXE )
					{
						damage++;
					}

					bool gungnir = false;
					if ( myStats->weapon )
						if ( myStats->weapon->type == ARTIFACT_SPEAR )
						{
							gungnir = true;
						}
					if ( weaponskill >= PRO_SWORD && weaponskill < PRO_SHIELD && !gungnir )
					{
						int chance = 0;
						if ( weaponskill == PRO_POLEARM )
						{
							chance = (damage / 3) * (100 - myStats->PROFICIENCIES[weaponskill]) / 100.f;
						}
						else
						{
							chance = (damage / 2) * (100 - myStats->PROFICIENCIES[weaponskill]) / 100.f;
						}
						if ( chance > 0 )
						{
							damage = (damage - chance) + (rand() % chance) + 1;
						}
					}

					int olddamage = damage;
					damage *= std::max(charge, MAXCHARGE / 2) / ((double)(MAXCHARGE / 2));

					if ( myStats->weapon )
						if ( myStats->weapon->type == ARTIFACT_AXE )
							if ( rand() % 3 == 0 )
							{
								damage *= 2;    // Parashu sometimes doubles damage
							}
					hit.entity->modHP(-damage); // do the damage

					// write the obituary
					killedByMonsterObituary(hit.entity);

					// update enemy bar for attacker
					if ( !strcmp(hitstats->name, "") )
					{
						if ( hitstats->type < 21 ) //Original monster count
						{
							updateEnemyBar(this, hit.entity, language[90 + hitstats->type], hitstats->HP, hitstats->MAXHP);
						}
						else if ( hitstats->type >= 21 ) //New monsters
						{
							updateEnemyBar(this, hit.entity, language[2000 + (hitstats->type - 21)], hitstats->HP, hitstats->MAXHP);
						}
					}
					else
					{
						updateEnemyBar(this, hit.entity, hitstats->name, hitstats->HP, hitstats->MAXHP);
					}

					// damage weapon if applicable
					if ( (rand() % 4 == 0 && damage == 0) || (rand() % 50 == 0 && damage > 0) )
					{
						if ( myStats->weapon != NULL )
						{
							bool artifactWeapon = false;
							if ( myStats->weapon->type == ARTIFACT_AXE )
							{
								artifactWeapon = true;
							}
							if ( myStats->weapon->type == ARTIFACT_MACE )
							{
								artifactWeapon = true;
							}
							if ( myStats->weapon->type == ARTIFACT_SPEAR )
							{
								artifactWeapon = true;
							}
							if ( myStats->weapon->type == ARTIFACT_SWORD )
							{
								artifactWeapon = true;
							}
							if ( !artifactWeapon )
							{
								if ( player == clientnum || player < 0 )
								{
									if ( myStats->weapon->count > 1 )
									{
										newItem(myStats->weapon->type, myStats->weapon->status, myStats->weapon->beatitude, myStats->weapon->count - 1, myStats->weapon->appearance, myStats->weapon->identified, &myStats->inventory);
									}
								}
								myStats->weapon->count = 1;
								myStats->weapon->status = static_cast<Status>(myStats->weapon->status - 1);
								if ( myStats->weapon->status != BROKEN )
								{
									messagePlayer(player, language[679]);
								}
								else
								{
									playSoundEntity(this, 76, 64);
									messagePlayer(player, language[680]);
								}
								if ( player > 0 && multiplayer == SERVER )
								{
									strcpy((char*)net_packet->data, "ARMR");
									net_packet->data[4] = 5;
									net_packet->data[5] = myStats->weapon->status;
									net_packet->address.host = net_clients[player - 1].host;
									net_packet->address.port = net_clients[player - 1].port;
									net_packet->len = 6;
									sendPacketSafe(net_sock, -1, net_packet, player - 1);
								}
							}
						}
					}

					// damage opponent armor if applicable
					Item* armor = NULL;
					int armornum = 0;
					if ( damage > 0 && ((rand() % 25 == 0 && weaponskill != PRO_MACE) || (rand() % 10 == 0 && weaponskill == PRO_MACE)) )
					{
						switch ( rand() % 6 )
						{
							case 0:
								armor = hitstats->helmet;
								armornum = 0;
								break;
							case 1:
								armor = hitstats->breastplate;
								armornum = 1;
								break;
							case 2:
								armor = hitstats->gloves;
								armornum = 2;
								break;
							case 3:
								armor = hitstats->shoes;
								armornum = 3;
								break;
							case 4:
								armor = hitstats->shield;
								armornum = 4;
								break;
							case 5:
								armor = hitstats->cloak;
								armornum = 6;
								break;
							default:
								break;
						}
					}
					else
					{
						if ( hitstats->shield != NULL )
						{
							if ( itemCategory(hitstats->shield) == ARMOR )
							{
								if ( (rand() % 10 == 0 && damage > 0) || (damage == 0 && rand() % 3 == 0) )
								{
									hit.entity->increaseSkill(PRO_SHIELD); // increase shield skill
								}
							}
						}
					}
					if ( hitstats->defending && rand() % 10 == 0 && !armor )
					{
						armor = hitstats->shield;
						armornum = 4;
					}
					if ( armor != NULL )
					{
						if ( playerhit == clientnum || playerhit < 0 )
						{
							if ( armor->count > 1 )
							{
								newItem(armor->type, armor->status, armor->beatitude, armor->count - 1, armor->appearance, armor->identified, &hitstats->inventory);
							}
						}
						armor->count = 1;
						armor->status = static_cast<Status>(armor->status - 1);
						if ( armor->status > BROKEN )
						{
							messagePlayer(playerhit, language[681], armor->getName());
						}
						else
						{
							playSoundEntity(hit.entity, 76, 64);
							messagePlayer(playerhit, language[682], armor->getName());
						}
						if ( playerhit > 0 && multiplayer == SERVER )
						{
							strcpy((char*)net_packet->data, "ARMR");
							net_packet->data[4] = armornum;
							net_packet->data[5] = armor->status;
							net_packet->address.host = net_clients[playerhit - 1].host;
							net_packet->address.port = net_clients[playerhit - 1].port;
							net_packet->len = 6;
							sendPacketSafe(net_sock, -1, net_packet, playerhit - 1);
						}
					}

					// special weapon effects
					if ( myStats->weapon )
					{
						if ( myStats->weapon->type == ARTIFACT_SWORD )
						{
							if ( hit.entity->flags[BURNABLE] )
							{
								if ( hitstats )
								{
									hitstats->poisonKiller = uid;
								}
								hit.entity->flags[BURNING] = true;
								if ( playerhit > 0 && multiplayer == SERVER )
								{
									messagePlayer(playerhit, language[683]);
									serverUpdateEntityFlag(hit.entity, BURNING);
								}
							}
						}
					}

					// special monster effects
					if ( damage > 0 && rand() % 4 == 0 )
					{
						int armornum = 0;
						Item* armor = NULL;
						int armorstolen = rand() % 9;
						switch ( myStats->type )
						{
							case SCORPION:
								hitstats->EFFECTS[EFF_PARALYZED] = true;
								hitstats->EFFECTS_TIMERS[EFF_PARALYZED] = 120;
								messagePlayer(playerhit, language[684]);
								messagePlayer(playerhit, language[685]);
								serverUpdateEffects(playerhit);
								break;
							case SPIDER:
								hitstats->EFFECTS[EFF_POISONED] = true;
								hitstats->EFFECTS_TIMERS[EFF_POISONED] = 600;
								messagePlayer(playerhit, language[686]);
								messagePlayer(playerhit, language[687]);
								serverUpdateEffects(playerhit);
								break;
							case SUCCUBUS:
								switch ( armorstolen )
								{
									case 0:
										armor = hitstats->helmet;
										armornum = 0;
										break;
									case 1:
										armor = hitstats->breastplate;
										armornum = 1;
										break;
									case 2:
										armor = hitstats->gloves;
										armornum = 2;
										break;
									case 3:
										armor = hitstats->shoes;
										armornum = 3;
										break;
									case 4:
										armor = hitstats->shield;
										armornum = 4;
										break;
									case 5:
										armor = hitstats->cloak;
										armornum = 6;
										break;
									case 6:
										armor = hitstats->amulet;
										armornum = 7;
										break;
									case 7:
										armor = hitstats->ring;
										armornum = 8;
										break;
									case 8:
										armor = hitstats->mask;
										armornum = 9;
										break;
									default:
										break;
								}
								if ( armor != NULL )
								{
									if ( playerhit == clientnum || playerhit < 0 )
									{
										if ( armor->count > 1 )
										{
											newItem(armor->type, armor->status, armor->beatitude, armor->count - 1, armor->appearance, armor->identified, &hitstats->inventory);
										}
									}
									armor->count = 1;
									messagePlayer(playerhit, language[688], armor->getName());
									newItem(armor->type, armor->status, armor->beatitude, armor->count, armor->appearance, armor->identified, &myStats->inventory);
									Item** slot = itemSlot(hitstats, armor);
									if ( slot )
									{
										*slot = NULL;
									}
									if ( armor->node )
									{
										list_RemoveNode(armor->node);
									}
									else
									{
										free(armor);
									}
									if ( playerhit > 0 && multiplayer == SERVER )
									{
										strcpy((char*)net_packet->data, "STLA");
										net_packet->data[4] = armornum;
										net_packet->address.host = net_clients[playerhit - 1].host;
										net_packet->address.port = net_clients[playerhit - 1].port;
										net_packet->len = 5;
										sendPacketSafe(net_sock, -1, net_packet, playerhit - 1);
									}
									teleportRandom();

									// the succubus loses interest after this
									monster_state = 0;
									monster_target = 0;
								}
								break;
							default:
								break;
						}
					}

					// send messages
					if ( !strcmp(hitstats->name, "") )
					{
						if ( hitstats->HP > 0 )
						{
							if ( damage > olddamage )
							{
								Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
								if ( hitstats->type < 21 ) //Original monster count
								{
									messagePlayerColor(player, color, language[689], language[90 + hitstats->type]);
								}
								else if ( hitstats->type >= 21 ) //New monsters
								{
									messagePlayerColor(player, color, language[689], language[2000 + (hitstats->type - 21)]);
								}	
							}
							else
							{
								Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
								if ( hitstats->type < 21 ) //Original monster count
								{
									messagePlayerColor(player, color, language[690], language[90 + hitstats->type]);
								}
								else if ( hitstats->type >= 21 ) //New monsters
								{
									messagePlayerColor(player, color, language[690], language[2000 + (hitstats->type - 21)]);
								}
							}
							if ( damage == 0 )
							{
								messagePlayer(player, language[691]);
							}
						}
						else
						{
							Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
							if ( hitstats->type < 21 ) //Original monster count
							{
								messagePlayerColor(player, color, language[692], language[90 + hitstats->type]);
							}
							else if ( hitstats->type >= 21 ) //New monsters
							{
								messagePlayerColor(player, color, language[692], language[2000 + (hitstats->type - 21)]);
							}
							awardXP( hit.entity, true, true );
						}
					}
					else
					{
						if ( hitstats->HP > 0 )
						{
							if ( damage > olddamage )
							{
								Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
								messagePlayerColor(player, color, language[693], hitstats->name);
							}
							else
							{
								Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
								messagePlayerColor(player, color, language[694], hitstats->name);
							}
							if ( damage == 0 )
							{
								if ( hitstats->sex )
								{
									messagePlayer(player, language[695]);
								}
								else
								{
									messagePlayer(player, language[696]);
								}
							}
						}
						else
						{
							Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
							messagePlayerColor(player, color, language[697], hitstats->name);
							awardXP( hit.entity, true, true );
						}
					}
					if ( playerhit > 0 && multiplayer == SERVER )
					{
						strcpy((char*)net_packet->data, "UPHP");
						SDLNet_Write32((Uint32)hitstats->HP, &net_packet->data[4]);
						SDLNet_Write32((Uint32)myStats->type, &net_packet->data[8]);
						net_packet->address.host = net_clients[playerhit - 1].host;
						net_packet->address.port = net_clients[playerhit - 1].port;
						net_packet->len = 12;
						sendPacketSafe(net_sock, -1, net_packet, playerhit - 1);
					}
					else if ( playerhit == 0 )
					{
						if ( damage > 0 )
						{
							camera_shakex += .1;
							camera_shakey += 10;
						}
						else
						{
							camera_shakex += .05;
							camera_shakey += 5;
						}
					}
					if ( !strcmp(myStats->name, "") )
					{
						Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
						if ( myStats->type < 21 ) //Original monster count
						{
							messagePlayerColor(playerhit, color, language[698], language[90 + myStats->type], language[132 + myStats->type]);
						}
						else if ( myStats->type >= 21 ) //New monsters
						{
							messagePlayerColor(playerhit, color, language[698], language[2000 + (myStats->type - 21)], language[2100 + (myStats->type - 21)]);
						}
					}
					else
					{
						Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
						if ( myStats->type < 21 ) //Original monster count
						{
							messagePlayerColor(playerhit, color, language[699], myStats->name, language[132 + myStats->type]);
						}
						else if ( myStats->type >= 21 ) //New monsters
						{
							messagePlayerColor(playerhit, color, language[699], myStats->name, language[2100 + (myStats->type - 21)]);
						}
					}
					if ( damage > 0 )
					{
						Entity* gib = spawnGib(hit.entity);
						serverSpawnGibForClient(gib);
					}
					else
					{
						messagePlayer(playerhit, language[700]);
					}
					playSoundEntity(hit.entity, 28, 64);

					// chance of bleeding
					if ( gibtype[(int)hitstats->type] == 1 )
					{
						if ( hitstats->HP > 5 && damage > 0 && !hitstats->EFFECTS[EFF_BLEEDING] )
						{
							if ( (rand() % 20 == 0 && weaponskill != PRO_SWORD) || (rand() % 10 == 0 && weaponskill == PRO_SWORD) )
							{
								hitstats->EFFECTS_TIMERS[EFF_BLEEDING] = std::max(480 + rand() % 360 - hit.entity->getCON() * 100, 120);
								hitstats->EFFECTS[EFF_BLEEDING] = true;
								if ( player > 0 && multiplayer == SERVER )
								{
									serverUpdateEffects(player);
								}
								if ( playerhit >= 0 )
								{
									Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
									messagePlayerColor(playerhit, color, language[701]);
								}
								else
								{
									Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
									if ( !strcmp(hitstats->name, "") )
									{
										if ( hitstats->type < 21 ) //Original monster count
										{
											messagePlayerColor(player, color, language[702], language[90 + hitstats->type]);
										}
										else if ( hitstats->type >= 21 ) //New monsters
										{
											messagePlayerColor(player, color, language[702], language[2000 + (hitstats->type - 21)]);
										}
									}
									else
									{
										messagePlayerColor(player, color, language[703], hitstats->name);
									}
								}
							}
						}
					}
				}
			}
		}
		else
		{
			if ( dist != STRIKERANGE )
			{
				// hit a wall
				if ( myStats->weapon != NULL )
				{
					if ( myStats->weapon->type == TOOL_PICKAXE )
					{
						if ( hit.mapx >= 1 && hit.mapx < map.width - 1 && hit.mapy >= 1 && hit.mapy < map.height - 1 )
						{
							playSoundPos(hit.x, hit.y, 67, 128);

							// spawn several rock items
							i = 8 + rand() % 4;
							for ( c = 0; c < i; c++ )
							{
								entity = newEntity(-1, 1, map.entities);
								entity->flags[INVISIBLE] = true;
								entity->flags[UPDATENEEDED] = true;
								entity->x = hit.mapx * 16 + 4 + rand() % 8;
								entity->y = hit.mapy * 16 + 4 + rand() % 8;
								entity->z = -6 + rand() % 12;
								entity->sizex = 4;
								entity->sizey = 4;
								entity->yaw = rand() % 360 * PI / 180;
								entity->vel_x = (rand() % 20 - 10) / 10.0;
								entity->vel_y = (rand() % 20 - 10) / 10.0;
								entity->vel_z = -.25 - (rand() % 5) / 10.0;
								entity->flags[PASSABLE] = true;
								entity->behavior = &actItem;
								entity->flags[USERFLAG1] = true; // no collision: helps performance
								entity->skill[10] = GEM_ROCK;    // type
								entity->skill[11] = WORN;        // status
								entity->skill[12] = 0;           // beatitude
								entity->skill[13] = 1;           // count
								entity->skill[14] = 0;           // appearance
								entity->skill[15] = false;       // identified
							}

							map.tiles[OBSTACLELAYER + hit.mapy * MAPLAYERS + hit.mapx * MAPLAYERS * map.height] = 0;
							// send wall destroy info to clients
							if ( multiplayer == SERVER )
							{
								for ( c = 0; c < MAXPLAYERS; c++ )
								{
									if ( client_disconnected[c] == true )
									{
										continue;
									}
									strcpy((char*)net_packet->data, "WALD");
									SDLNet_Write16((Uint16)hit.mapx, &net_packet->data[4]);
									SDLNet_Write16((Uint16)hit.mapy, &net_packet->data[6]);
									net_packet->address.host = net_clients[c - 1].host;
									net_packet->address.port = net_clients[c - 1].port;
									net_packet->len = 8;
									sendPacketSafe(net_sock, -1, net_packet, c - 1);
								}
							}
							if ( rand() % 2 )
							{
								myStats->weapon->status = static_cast<Status>(myStats->weapon->status - 1);
								if ( myStats->weapon->status == BROKEN )
								{
									messagePlayer(player, language[704]);
									playSoundEntity(this, 76, 64);
								}
								else
								{
									messagePlayer(player, language[705]);
								}
								if ( player > 0 && multiplayer == SERVER )
								{
									strcpy((char*)net_packet->data, "ARMR");
									net_packet->data[4] = 5;
									net_packet->data[5] = myStats->weapon->status;
									net_packet->address.host = net_clients[player - 1].host;
									net_packet->address.port = net_clients[player - 1].port;
									net_packet->len = 6;
									sendPacketSafe(net_sock, -1, net_packet, player - 1);
								}
							}

							// Update the paths so that monsters know they can walk through it
							generatePathMaps();
						}
						else
						{
							spawnBang(hit.x - cos(yaw) * 2, hit.y - sin(yaw) * 2, 0);
							messagePlayer(player, language[706]);
						}
					}
					else
					{
						// bang
						spawnBang(hit.x - cos(yaw) * 2, hit.y - sin(yaw) * 2, 0);
					}
				}
				else
				{
					// bang
					//spawnBang(hit.x - cos(my->yaw)*2,hit.y - sin(my->yaw)*2,0);
					playSoundPos(hit.x, hit.y, 183, 64);
				}
			}
		}
	}
	else
	{
		if ( player == -1 )
		{
			return;    // clients are NOT supposed to invoke monster attacks in the gamestate!
		}
		strcpy((char*)net_packet->data, "ATAK");
		net_packet->data[4] = player;
		net_packet->data[5] = pose;
		net_packet->data[6] = charge;
		net_packet->address.host = net_server.host;
		net_packet->address.port = net_server.port;
		net_packet->len = 7;
		sendPacketSafe(net_sock, -1, net_packet, 0);
	}
}

/*-------------------------------------------------------------------------------

	AC

	Returns armor class value from a Stat instance

-------------------------------------------------------------------------------*/

int AC(Stat* stat)
{
	if (!stat)
	{
		return 0;
	}

	int armor = stat->CON;

	if (stat->helmet)
	{
		armor += stat->helmet->armorGetAC();
	}
	if (stat->breastplate)
	{
		armor += stat->breastplate->armorGetAC();
	}
	if (stat->gloves)
	{
		armor += stat->gloves->armorGetAC();
	}
	if (stat->shoes)
	{
		armor += stat->shoes->armorGetAC();
	}
	if (stat->shield)
	{
		armor += stat->shield->armorGetAC();
	}
	if (stat->cloak)
	{
		armor += stat->cloak->armorGetAC();
	}
	if (stat->ring)
	{
		armor += stat->ring->armorGetAC();
	}

	if ( stat->shield )
	{
		int shieldskill = stat->PROFICIENCIES[PRO_SHIELD] / 25;
		armor += shieldskill;
		if ( stat->defending )
		{
			armor += 5 + stat->PROFICIENCIES[PRO_SHIELD] / 5;
		}
	}

	return armor;
}

/*-------------------------------------------------------------------------------

	Entity::teleport

	Teleports the given entity to the given (x, y) location on the map,
	in map coordinates. Will not teleport if the destination is an obstacle.

-------------------------------------------------------------------------------*/

void Entity::teleport(int tele_x, int tele_y)
{
	int player = -1;

	if (behavior == &actPlayer)
	{
		player = skill[2];
	}

	if ( strstr(map.name, "Minotaur") || checkObstacle((tele_x << 4) + 8, (tele_y << 4) + 8, this, NULL) )
	{
		messagePlayer(player, language[707]);
		return;
	}

	// play sound effect
	playSoundEntity(this, 77, 64);

	// relocate entity
	double oldx = x;
	double oldy = y;
	x = (tele_x << 4) + 8;
	y = (tele_y << 4) + 8;
	if ( entityInsideSomething(this) )
	{
		x = oldx;
		y = oldy;
		if ( multiplayer == SERVER && player > 0 )
		{
			messagePlayer(player, language[707]);
		}
		return;
	}
	if ( player > 0 && multiplayer == SERVER )
	{
		strcpy((char*)net_packet->data, "TELE");
		net_packet->data[4] = x;
		net_packet->data[5] = y;
		net_packet->address.host = net_clients[player - 1].host;
		net_packet->address.port = net_clients[player - 1].port;
		net_packet->len = 6;
		sendPacketSafe(net_sock, -1, net_packet, player - 1);
	}

	// play second sound effect
	playSoundEntity(this, 77, 64);
}

/*-------------------------------------------------------------------------------

	Entity::teleportRandom

	Teleports the given entity to a random location on the map.

-------------------------------------------------------------------------------*/

void Entity::teleportRandom()
{
	int numlocations = 0;
	int pickedlocation;
	int player = -1;

	if (behavior == &actPlayer )
	{
		player = skill[2];
	}
	for (int iy = 0; iy < map.height; ++iy )
	{
		for (int ix = 0; ix < map.width; ++ix )
		{
			if ( !checkObstacle((ix << 4) + 8, (iy << 4) + 8, this, NULL) )
			{
				numlocations++;
			}
		}
	}
	if ( numlocations == 0 )
	{
		messagePlayer(player, language[708]);
		return;
	}
	pickedlocation = rand() % numlocations;
	numlocations = 0;
	for (int iy = 0; iy < map.height; iy++ )
	{
		for (int ix = 0; ix < map.width; ix++ )
		{
			if ( !checkObstacle((ix << 4) + 8, (iy << 4) + 8, this, NULL) )
			{
				if ( numlocations == pickedlocation )
				{
					teleport(ix, iy);
					return;
				}
				numlocations++;
			}
		}
	}
}

/*-------------------------------------------------------------------------------

	Entity::awardXP

	Awards XP to the dest (ie killer) entity from the src (ie killed) entity

-------------------------------------------------------------------------------*/

void Entity::awardXP(Entity* src, bool share, bool root)
{
	if ( !src )
	{
		return;
	}

	Stat* destStats = getStats();
	Stat* srcStats = src->getStats();

	if ( !destStats || !srcStats )
	{
		return;
	}

	int player = -1;
	if ( behavior == &actPlayer )
	{
		player = skill[2];
	}

	// calculate XP gain
	int xpGain = 10 + rand() % 10 + std::max(0, srcStats->LVL - destStats->LVL) * 10;

	// save hit struct
	hit_t tempHit;
	tempHit.entity = hit.entity;
	tempHit.mapx = hit.mapx;
	tempHit.mapy = hit.mapy;
	tempHit.side = hit.side;
	tempHit.x = hit.x;
	tempHit.y = hit.y;

	// divide shares
	if ( player >= 0 )
	{
		int numshares = 0;
		Entity* shares[MAXPLAYERS];
		int c;

		for ( c = 0; c < MAXPLAYERS; c++ )
		{
			shares[c] = NULL;
		}

		// find other players to divide shares with
		node_t* node;
		for ( node = map.entities->first; node != NULL; node = node->next )
		{
			Entity* entity = (Entity*)node->element;
			if ( entity == this )
			{
				continue;
			}
			if ( entity->behavior == &actPlayer )
			{
				double tangent = atan2( entity->y - src->y, entity->x - src->x );
				lineTrace(src, src->x, src->y, tangent, XPSHARERANGE, 0, false);

				if ( hit.entity == entity )
				{
					numshares++;
					shares[numshares] = entity;
					if ( numshares == MAXPLAYERS - 1 )
					{
						break;
					}
				}
			}
		}

		// divide value of each share
		if ( numshares )
		{
			xpGain /= numshares;
		}

		// award XP to everyone else in the group
		if ( share )
		{
			for ( c = 0; c < MAXPLAYERS; c++ )
			{
				if ( shares[c] )
				{
					shares[c]->awardXP(src, false, false);
				}
			}
		}
	}

	// award XP to main victor
	destStats->EXP += xpGain;

	// award bonus XP and update kill counters
	if ( player >= 0 )
	{
		if ( player == 0 )
		{
			kills[srcStats->type]++;
		}
		else if ( multiplayer == SERVER && player > 0 )
		{
			// inform client of kill
			strcpy((char*)net_packet->data, "MKIL");
			net_packet->data[4] = srcStats->type;
			net_packet->address.host = net_clients[player - 1].host;
			net_packet->address.port = net_clients[player - 1].port;
			net_packet->len = 5;
			sendPacketSafe(net_sock, -1, net_packet, player - 1);

			// update client attributes
			strcpy((char*)net_packet->data, "ATTR");
			net_packet->data[4] = clientnum;
			net_packet->data[5] = (Sint8)destStats->STR;
			net_packet->data[6] = (Sint8)destStats->DEX;
			net_packet->data[7] = (Sint8)destStats->CON;
			net_packet->data[8] = (Sint8)destStats->INT;
			net_packet->data[9] = (Sint8)destStats->PER;
			net_packet->data[10] = (Sint8)destStats->CHR;
			net_packet->data[11] = (Sint8)destStats->EXP;
			net_packet->data[12] = (Sint8)destStats->LVL;
			SDLNet_Write16((Sint16)destStats->HP, &net_packet->data[13]);
			SDLNet_Write16((Sint16)destStats->MAXHP, &net_packet->data[15]);
			SDLNet_Write16((Sint16)destStats->MP, &net_packet->data[17]);
			SDLNet_Write16((Sint16)destStats->MAXMP, &net_packet->data[19]);
			net_packet->address.host = net_clients[player - 1].host;
			net_packet->address.port = net_clients[player - 1].port;
			net_packet->len = 21;
			sendPacketSafe(net_sock, -1, net_packet, player - 1);
		}
	}
	else
	{
		Entity* leader;

		// NPCs with leaders award equal XP to their master (so NPCs don't steal XP gainz)
		if ( (leader = uidToEntity(destStats->leader_uid)) != NULL )
		{
			leader->increaseSkill(PRO_LEADERSHIP);
			leader->awardXP(src, true, false);
		}
	}

	// restore hit struct
	if ( root )
	{
		hit.entity = tempHit.entity;
		hit.mapx = tempHit.mapx;
		hit.mapy = tempHit.mapy;
		hit.side = tempHit.side;
		hit.x = tempHit.x;
		hit.y = tempHit.y;
	}
}

/*-------------------------------------------------------------------------------

	Entity::checkEnemy

	Returns true if my and your are enemies, otherwise returns false

-------------------------------------------------------------------------------*/

bool Entity::checkEnemy(Entity* your)
{
	if (!your)
	{
		return false;
	}

	bool result;

	Stat* myStats = getStats();
	Stat* yourStats = your->getStats();

	if ( !myStats || !yourStats )
	{
		return false;
	}
	if ( everybodyfriendly )   // friendly monsters mode
	{
		return false;
	}

	if ( (your->behavior == &actPlayer || your->behavior == &actPlayerLimb) && (behavior == &actPlayer || behavior == &actPlayerLimb) )
	{
		return false;
	}

	// if you have a leader, check whether we are enemies instead
	Entity* yourLeader = NULL;
	if ( yourStats->leader_uid )
	{
		yourLeader = uidToEntity(yourStats->leader_uid);
	}
	if ( yourLeader )
	{
		Stat* yourLeaderStats = yourLeader->getStats();
		if ( yourLeaderStats )
		{
			if ( yourLeader == this )
			{
				return false;
			}
			else
			{
				return checkEnemy(yourLeader);
			}
		}
	}

	// first find out if I have a leader
	Entity* myLeader = NULL;
	if ( myStats->leader_uid )
	{
		myLeader = uidToEntity(myStats->leader_uid);
	}
	if ( myLeader )
	{
		Stat* myLeaderStats = myLeader->getStats();
		if ( myLeaderStats )
		{
			if ( myLeader == your )
			{
				result = false;
			}
			else
			{
				return myLeader->checkEnemy(your);
			}
		}
		else
		{
			// invalid leader, default to allegiance table
			result = swornenemies[myStats->type][yourStats->type];
		}
	}
	else
	{
		node_t* t_node;
		bool foundFollower = false;
		for ( t_node = myStats->FOLLOWERS.first; t_node != NULL; t_node = t_node->next )
		{
			Uint32* uid = (Uint32*)t_node->element;
			if ( *uid == your->uid )
			{
				foundFollower = true;
				result = false;
				break;
			}
		}
		if ( !foundFollower )
		{
			// no leader, default to allegiance table
			result = swornenemies[myStats->type][yourStats->type];
		}
	}

	// confused monsters mistake their allegiances
	if ( myStats->EFFECTS[EFF_CONFUSED] )
	{
		result = (result == false);
	}

	return result;
}

/*-------------------------------------------------------------------------------

	Entity::checkFriend

	Returns true if my and your are friends, otherwise returns false

-------------------------------------------------------------------------------*/

bool Entity::checkFriend(Entity* your)
{
	bool result;

	if (!your)
	{
		return false;    //Equivalent to if (!myStats || !yourStats)
	}

	Stat* myStats = getStats();
	Stat* yourStats = your->getStats();

	if ( !myStats || !yourStats )
	{
		return false;
	}

	if ( (your->behavior == &actPlayer || your->behavior == &actPlayerLimb) && (behavior == &actPlayer || behavior == &actPlayerLimb) )
	{
		return true;
	}

	// if you have a leader, check whether we are friends instead
	Entity* yourLeader = NULL;
	if ( yourStats->leader_uid )
	{
		yourLeader = uidToEntity(yourStats->leader_uid);
	}
	if ( yourLeader )
	{
		Stat* yourLeaderStats = yourLeader->getStats();
		if ( yourLeaderStats )
		{
			if ( yourLeader == this )
			{
				return true;
			}
			else
			{
				return checkFriend(yourLeader);
			}
		}
	}

	// first find out if I have a leader
	Entity* myLeader = NULL;
	if ( myStats->leader_uid )
	{
		myLeader = uidToEntity(myStats->leader_uid);
	}
	if ( myLeader )
	{
		Stat* myLeaderStats = myLeader->getStats();
		if ( myLeaderStats )
		{
			if ( myLeader == your )
			{
				result = true;
			}
			else
			{
				return myLeader->checkFriend(your);
			}
		}
		else
		{
			// invalid leader, default to allegiance table
			result = monsterally[myStats->type][yourStats->type];
		}
	}
	else
	{
		node_t* t_node;
		bool foundFollower = false;
		for ( t_node = myStats->FOLLOWERS.first; t_node != NULL; t_node = t_node->next )
		{
			Uint32* uid = (Uint32*)t_node->element;
			if ( *uid == your->uid )
			{
				foundFollower = true;
				result = true;
				break;
			}
		}
		if ( !foundFollower )
		{
			// no leader, default to allegiance table
			result = monsterally[myStats->type][yourStats->type];
		}
	}

	return result;
}
