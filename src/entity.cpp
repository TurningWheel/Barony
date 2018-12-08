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
#include "book.hpp"
#ifdef STEAMWORKS
#include <steam/steam_api.h>
#endif
#include "player.hpp"
#include "scores.hpp"
#ifdef __ARM_NEON__
#include <arm_neon.h>
#endif

/*-------------------------------------------------------------------------------

Entity::Entity)

Construct an Entity

-------------------------------------------------------------------------------*/

Entity::Entity(Sint32 in_sprite, Uint32 pos, list_t* entlist, list_t* creaturelist) :
	char_gonnavomit(skill[26]),
	char_heal(skill[22]),
	char_energize(skill[23]),
	char_torchtime(skill[25]),
	char_poison(skill[21]),
	char_fire(skill[36]),
	chanceToPutOutFire(skill[37]),
	circuit_status(skill[28]),
	switch_power(skill[0]),
	chestInit(skill[0]),
	chestStatus(skill[1]),
	chestHealth(skill[3]),
	chestLocked(skill[4]),
	chestOpener(skill[5]),
	chestLidClicked(skill[6]),
	chestAmbience(skill[7]),
	chestMaxHealth(skill[8]),
	chestType(skill[9]),
	chestPreventLockpickCapstoneExploit(skill[10]),
	monsterState(skill[0]),
	monsterTarget(skill[1]),
	monsterTargetX(fskill[2]),
	monsterTargetY(fskill[3]),
	crystalInitialised(skill[1]),
	crystalTurning(skill[3]),
	crystalTurnStartDir(skill[4]),
	crystalGeneratedElectricityNodes(skill[5]),
	crystalNumElectricityNodes(skill[6]),
	crystalHoverDirection(skill[7]),
	crystalHoverWaitTimer(skill[8]),
	crystalTurnReverse(skill[9]),
	crystalSpellToActivate(skill[10]),
	crystalStartZ(fskill[0]),
	crystalMaxZVelocity(fskill[1]),
	crystalMinZVelocity(fskill[2]),
	crystalTurnVelocity(fskill[3]),
	monsterAnimationLimbDirection(skill[20]),
	monsterAnimationLimbOvershoot(skill[30]),
	monsterSpecialTimer(skill[29]),
	monsterSpecialState(skill[33]),
	monsterSpellAnimation(skill[31]),
	monsterFootstepType(skill[32]),
	monsterLookTime(skill[4]),
	monsterMoveTime(skill[6]),
	monsterLookDir(fskill[4]),
	monsterEntityRenderAsTelepath(skill[41]),
	playerLevelEntrySpeech(skill[18]),
	playerAliveTime(skill[12]),
	monsterAttack(skill[8]),
	monsterAttackTime(skill[9]),
	monsterArmbended(skill[10]),
	monsterWeaponYaw(fskill[5]),
	monsterShadowInitialMimic(skill[34]),
	monsterShadowDontChangeName(skill[35]),
	monsterLichFireMeleeSeq(skill[34]),
	monsterLichFireMeleePrev(skill[35]),
	monsterLichIceCastSeq(skill[34]),
	monsterLichIceCastPrev(skill[35]),
	monsterLichMagicCastCount(skill[37]),
	monsterLichMeleeSwingCount(skill[38]),
	monsterLichBattleState(skill[27]),
	monsterLichTeleportTimer(skill[40]),
	monsterLichAllyStatus(skill[18]),
	monsterLichAllyUID(skill[17]),
	monsterPathBoundaryXStart(skill[14]),
	monsterPathBoundaryYStart(skill[15]),
	monsterPathBoundaryXEnd(skill[16]),
	monsterPathBoundaryYEnd(skill[17]),
	monsterStoreType(skill[18]),
	monsterStrafeDirection(skill[39]),
	monsterPathCount(skill[38]),
	monsterAllyIndex(skill[42]),
	monsterAllyState(skill[43]),
	monsterAllyPickupItems(skill[44]),
	monsterAllyInteractTarget(skill[45]),
	monsterAllyClass(skill[46]),
	monsterDefend(skill[47]),
	monsterAllySpecial(skill[48]),
	monsterAllySpecialCooldown(skill[49]),
	monsterAllySummonRank(skill[50]),
	particleDuration(skill[0]),
	particleShrink(skill[1]),
	monsterHitTime(skill[7]),
	itemNotMoving(skill[18]),
	itemNotMovingClient(skill[19]),
	itemSokobanReward(skill[20]),
	itemOriginalOwner(skill[21]),
	itemStolen(skill[22]),
	gateInit(skill[1]),
	gateStatus(skill[3]),
	gateRattle(skill[4]),
	gateStartHeight(fskill[0]),
	gateVelZ(vel_z),
	gateInverted(skill[5]),
	leverStatus(skill[1]),
	leverTimerTicks(skill[3]),
	boulderTrapRefireAmount(skill[1]),
	boulderTrapRefireDelay(skill[3]),
	boulderTrapAmbience(skill[6]),
	boulderTrapFired(skill[0]),
	boulderTrapRefireCounter(skill[4]),
	boulderTrapPreDelay(skill[5]),
	boulderTrapRocksToSpawn(skill[7]),
	doorDir(skill[0]),
	doorInit(skill[1]),
	doorStatus(skill[3]),
	doorHealth(skill[4]),
	doorLocked(skill[5]),
	doorSmacked(skill[6]),
	doorTimer(skill[7]),
	doorOldStatus(skill[8]),
	doorMaxHealth(skill[9]),
	doorStartAng(fskill[0]),
	particleTimerDuration(skill[0]),
	particleTimerEndAction(skill[1]),
	particleTimerEndSprite(skill[3]),
	particleTimerCountdownAction(skill[4]),
	particleTimerCountdownSprite(skill[5]),
	particleTimerTarget(skill[6]),
	particleTimerPreDelay(skill[7]),
	particleTimerVariable1(skill[8]),
	pedestalHasOrb(skill[0]),
	pedestalOrbType(skill[1]),
	pedestalInvertedPower(skill[3]),
	pedestalInGround(skill[4]),
	pedestalInit(skill[5]),
	pedestalAmbience(skill[6]),
	pedestalLockOrb(skill[7]),
	orbInitialised(skill[1]),
	orbHoverDirection(skill[7]),
	orbHoverWaitTimer(skill[8]),
	orbStartZ(fskill[0]),
	orbMaxZVelocity(fskill[1]),
	orbMinZVelocity(fskill[2]),
	orbTurnVelocity(fskill[3]),
	portalAmbience(skill[0]),
	portalInit(skill[1]),
	portalNotSecret(skill[3]),
	portalVictoryType(skill[4]),
	portalFireAnimation(skill[5]),
	teleporterX(skill[0]),
	teleporterY(skill[1]),
	teleporterType(skill[3]),
	teleporterAmbience(skill[4]),
	spellTrapType(skill[0]),
	spellTrapRefire(skill[1]),
	spellTrapLatchPower(skill[3]),
	spellTrapFloorTile(skill[4]),
	spellTrapRefireRate(skill[5]),
	spellTrapAmbience(skill[6]),
	spellTrapInit(skill[7]),
	spellTrapCounter(skill[8]),
	spellTrapReset(skill[9]),
	ceilingTileModel(skill[0]),
	floorDecorationModel(skill[0]),
	floorDecorationRotation(skill[1]),
	floorDecorationHeightOffset(skill[3]),
	furnitureType(skill[0]),
	furnitureInit(skill[1]),
	furnitureDir(skill[3]),
	furnitureHealth(skill[4]),
	furnitureMaxHealth(skill[9]),
	pistonCamDir(skill[0]),
	pistonCamTimer(skill[1]),
	pistonCamRotateSpeed(fskill[0]),
	arrowPower(skill[3]),
	arrowPoisonTime(skill[4]),
	arrowArmorPierce(skill[5]),
	actmagicIsVertical(skill[6]),
	actmagicIsOrbiting(skill[7]),
	actmagicOrbitDist(skill[8]),
	actmagicOrbitVerticalDirection(skill[9]),
	actmagicOrbitLifetime(skill[10]),
	actmagicMirrorReflected(skill[11]),
	actmagicMirrorReflectedCaster(skill[12]),
	actmagicCastByMagicstaff(skill[13]),
	actmagicOrbitVerticalSpeed(fskill[2]),
	actmagicOrbitStartZ(fskill[3]),
	goldAmount(skill[0]),
	goldAmbience(skill[1]),
	goldSokoban(skill[2]),
	interactedByMonster(skill[47]),
	soundSourceFired(skill[0]),
	soundSourceToPlay(skill[1]),
	soundSourceVolume(skill[2]),
	soundSourceLatchOn(skill[3]),
	soundSourceDelay(skill[4]),
	soundSourceDelayCounter(skill[5]),
	soundSourceOrigin(skill[6]),
	lightSourceBrightness(skill[0]),
	lightSourceAlwaysOn(skill[1]),
	lightSourceInvertPower(skill[2]),
	lightSourceLatchOn(skill[3]),
	lightSourceRadius(skill[4]),
	lightSourceFlicker(skill[5]),
	lightSourceDelay(skill[6]),
	lightSourceDelayCounter(skill[7]),
	textSourceColorRGB(skill[0]),
	textSourceVariables4W(skill[1]),
	textSourceDelay(skill[2]),
	textSource3(skill[3]),
	textSourceBegin(skill[4]),
	signalActivateDelay(skill[1]),
	signalTimerInterval(skill[2]),
	signalTimerRepeatCount(skill[3]),
	signalTimerLatchInput(skill[4]),
	signalInputDirection(skill[5]),
	effectPolymorph(skill[50])
{
	int c;
	// add the entity to the entity list
	if ( !pos )
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

	myCreatureListNode = nullptr;
	if ( creaturelist )
	{
		addToCreatureList(creaturelist);
	}
	myTileListNode = nullptr;

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
	light = nullptr;
	string = nullptr;
	children.first = nullptr;
	children.last = nullptr;
	//this->magic_effects = (list_t *) malloc(sizeof(list_t));
	//this->magic_effects->first = NULL; this->magic_effects->last = NULL;
	for ( c = 0; c < NUMENTITYSKILLS; ++c )
	{
		skill[c] = 0;
	}
	for ( c = 0; c < NUMENTITYFSKILLS; ++c )
	{
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
			map.entities_map.insert({ uid, mynode });
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
	behavior = nullptr;
	ranbehavior = false;
	parent = 0;
	path = nullptr;
	monsterAllyIndex = -1; // set to -1 to not reference player indices 0-3.
	if ( checkSpriteType(this->sprite) > 1 )
	{
		setSpriteAttributes(this, nullptr, nullptr);
	}

	clientStats = nullptr;
	clientsHaveItsStats = false;
}

void Entity::setUID(Uint32 new_uid)
{
	if ( mynode->list == map.entities )
	{
		map.entities_map.erase(uid);
		map.entities_map.insert({ new_uid, mynode });
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

	//Remove me from the
	if ( myCreatureListNode )
	{
		list_RemoveNode(myCreatureListNode);
		myCreatureListNode = nullptr;
	}
	if ( myTileListNode )
	{
		list_RemoveNode(myTileListNode);
		myTileListNode = nullptr;
	}

	// alert clients of the entity's deletion
	if ( multiplayer == SERVER && !loading )
	{
		if ( mynode->list == map.entities && uid != 0 && flags[NOUPDATE] == false )
		{
			for ( i = 1; i < MAXPLAYERS; ++i )
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
	for ( i = 0; i < MAXPLAYERS; ++i )
	{
		if ( this == players[i]->entity )
		{
			players[i]->entity = nullptr;    //TODO: PLAYERSWAP VERIFY. Should this do anything to the player itself?
		}
	}
	// destroy my children
	list_FreeAll(&this->children);

	node = list_AddNodeLast(&entitiesdeleted);
	node->element = this;
	node->deconstructor = &emptyDeconstructor;

	if ( clientStats )
	{
		delete clientStats;
	}
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
			if ( hitstats->type < KOBOLD ) //Original monster count
			{
				snprintf(tempstr, 256, language[1509], language[90 + hitstats->type]);
			}
			else if ( hitstats->type >= KOBOLD ) //New monsters
			{
				snprintf(tempstr, 256, language[1509], language[2000 + (hitstats->type - KOBOLD)]);
			}
		}
		else
		{
			if ( hitstats->type < KOBOLD ) //Original monster count
			{
				snprintf(tempstr, 256, language[1510], language[90 + hitstats->type]);
			}
			else if ( hitstats->type >= KOBOLD ) //New monsters
			{
				snprintf(tempstr, 256, language[1510], language[2000 + (hitstats->type - KOBOLD)]);
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

Entity::entityLightAfterReductions

Returns new entities' illumination,  
after reductions depending on the entity stats and another entity observing

-------------------------------------------------------------------------------*/

int Entity::entityLightAfterReductions(Stat& myStats, Entity& observer)
{
	int player = -1;
	int light = entityLight(); // max 255 light to start with.
	if ( !isInvisible() )
	{
		if ( behavior == &actPlayer )
		{
			player = skill[2];
			if ( player > -1 )
			{
				if ( stats[player]->shield )
				{
					if ( itemCategory(stats[player]->shield) == ARMOR )
					{
						light -= 95;
					}
				}
				else
				{
					light -= 95;
				}
				if ( stats[player]->sneaking == 1 )
				{
					light -= 64;
				}
			}
		}
		// reduce light level 0-200 depending on target's stealth.
		// add light level 0-150 for PER 0-30
		light -= myStats.PROFICIENCIES[PRO_STEALTH] * 2 - observer.getPER() * 5;
	}
	else
	{
		light = TOUCHRANGE;
	}
	light = std::max(light, 0);
	return light;
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


	spell_t* invisibility_hijacked = nullptr; //If NULL, function proceeds as normal. If points to something, it ignores the invisibility timer since a spell is doing things. //TODO: Incorporate the spell into isInvisible() instead?
	spell_t* levitation_hijacked = nullptr; //If NULL, function proceeds as normal. If points to something, it ignore the levitation timer since a spell is doing things.
	spell_t* reflectMagic_hijacked = nullptr;
	spell_t* vampiricAura_hijacked = nullptr;
	//Handle magic effects (like invisibility)
	for ( node = myStats->magic_effects.first; node; node = node->next, ++count )
	{
		//printlog( "%s\n", "Potato.");
		//Handle magic effects.
		spell = (spell_t*)node->element;
		if ( !spell->sustain )
		{
			node_t* temp = NULL;
			if ( node->prev )
			{
				temp = node->prev;
			}
			else if ( node->next )
			{
				temp = node->next;
			}
			spell->magic_effects_node = NULL; //To prevent recursive removal, which results in a crash.
			if ( player > -1 && multiplayer == SERVER )
			{
				strcpy((char*)net_packet->data, "UNCH");
				net_packet->data[4] = player;
				SDLNet_Write32(spell->ID, &net_packet->data[5]);
				net_packet->address.host = net_clients[player - 1].host;
				net_packet->address.port = net_clients[player - 1].port;
				net_packet->len = 9;
				sendPacketSafe(net_sock, -1, net_packet, player - 1);
			}
			list_RemoveNode(node); //Bugger the spell.
			node = temp;
			if ( !node )
			{
				break; //Done with list. Stop.
			}
			continue; //Skip this spell.
		}

		switch ( spell->ID )
		{
			case SPELL_INVISIBILITY:
				invisibility_hijacked = spell;
				if ( !myStats->EFFECTS[EFF_INVISIBLE] )
				{
					for ( c = 0; c < numplayers; ++c )
					{
						if ( players[c] && players[c]->entity == uidToEntity(spell->caster) && players[c]->entity != nullptr )
						{
							messagePlayer(c, language[591]);    //If cure ailments or somesuch bombs the status effects.
						}
					}
					node_t* temp = nullptr;
					if ( node->prev )
					{
						temp = node->prev;
					}
					else if ( node->next )
					{
						temp = node->next;
					}
					list_RemoveNode(node); //Remove this here node.
					node = temp;
				}
				break;
			case SPELL_LEVITATION:
				levitation_hijacked = spell;
				if ( !myStats->EFFECTS[EFF_LEVITATING] )
				{
					for ( c = 0; c < numplayers; ++c )
					{
						if ( players[c] && players[c]->entity == uidToEntity(spell->caster) && players[c]->entity != nullptr )
						{
							messagePlayer(c, language[592]);
						}
					}
					node_t* temp = nullptr;
					if ( node->prev )
					{
						temp = node->prev;
					}
					else if ( node->next )
					{
						temp = node->next;
					}
					list_RemoveNode(node); //Remove this here node.
					node = temp;
				}
				break;
			case SPELL_REFLECT_MAGIC:
				reflectMagic_hijacked = spell;
				if ( !myStats->EFFECTS[EFF_MAGICREFLECT] )
				{
					for ( c = 0; c < numplayers; ++c )
					{
						if ( players[c] && players[c]->entity == uidToEntity(spell->caster) && players[c]->entity != nullptr )
						{
							messagePlayer(c, language[2446]);
						}
					}
					node_t* temp = nullptr;
					if ( node->prev )
					{
						temp = node->prev;
					}
					else if ( node->next )
					{
						temp = node->next;
					}
					list_RemoveNode(node); //Remove this here node.
					node = temp;
				}
				break;
			case SPELL_VAMPIRIC_AURA:
				vampiricAura_hijacked = spell;
				if ( !myStats->EFFECTS[EFF_VAMPIRICAURA] )
				{
					for ( c = 0; c < numplayers; ++c )
					{
						if ( players[c] && players[c]->entity == uidToEntity(spell->caster) && players[c]->entity != nullptr )
						{
							messagePlayer(c, language[2447]);
						}
					}
					node_t* temp = nullptr;
					if ( node->prev )
					{
						temp = node->prev;
					}
					else if ( node->next )
					{
						temp = node->next;
					}
					list_RemoveNode(node); //Remove this here node.
					node = temp;
				}
				break;
			default:
				//Unknown spell, undefined effect. Like, say, a fireball spell wound up in here for some reason. That's a nono.
				printlog("[entityEffectTimes] Warning: magic_effects spell that's not relevant. Should not be in the magic_effects list!\n");
				list_RemoveNode(node);
		}

		if ( !node )
		{
			break;    //BREAK OUT. YEAAAAAH. Because otherwise it crashes.
		}
	}
	if ( count )
	{
		//printlog( "Number of magic effects spells: %d\n", count); //Debugging output.
	}

	bool dissipate = true;
	bool updateClient = false;

	for ( c = 0; c < NUMEFFECTS; c++ )
	{
		if ( myStats->EFFECTS_TIMERS[c] > 0 )
		{
			myStats->EFFECTS_TIMERS[c]--;
			if ( c == EFF_POLYMORPH )
			{
				if ( myStats->EFFECTS_TIMERS[c] == TICKS_PER_SECOND * 15 )
				{
					playSoundPlayer(player, 32, 128);
					messagePlayer(player, language[3193]);
				}
			}
			if ( myStats->EFFECTS_TIMERS[c] == 0 )
			{
				myStats->EFFECTS[c] = false;
				switch ( c )
				{
					case EFF_ASLEEP:
						messagePlayer(player, language[593]);
						if ( monsterAllyGetPlayerLeader() && monsterAllySpecial == ALLY_SPECIAL_CMD_REST )
						{
							monsterAllySpecial = ALLY_SPECIAL_CMD_NONE;
							myStats->EFFECTS[EFF_HP_REGEN] = false;
							myStats->EFFECTS_TIMERS[EFF_HP_REGEN] = 0;
						}
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
						if ( invisibility_hijacked )
						{
							bool sustained = false;
							Entity* caster = uidToEntity(invisibility_hijacked->caster);
							if ( caster )
							{
								//Deduct mana from caster. Cancel spell if not enough mana (simply leave sustained at false).
								bool deducted = caster->safeConsumeMP(1); //Consume 1 mana ever duration / mana seconds
								if ( deducted )
								{
									sustained = true;
									myStats->EFFECTS[c] = true;
									myStats->EFFECTS_TIMERS[c] = invisibility_hijacked->channel_duration;
								}
								else
								{
									int i = 0;
									for ( i = 0; i < 4; ++i )
									{
										if ( players[i]->entity == caster )
										{
											messagePlayer(i, language[598]);
										}
									}
									list_RemoveNode(invisibility_hijacked->magic_effects_node); //Remove it from the entity's magic effects. This has the side effect of removing it from the sustained spells list too.
																								//list_RemoveNode(invisibility_hijacked->sustain_node); //Remove it from the channeled spells list.
								}
							}
							if ( sustained )
							{
								dissipate = false;    //Sustained the spell, so do not stop being invisible.
							}
						}
						if ( dissipate )
						{
							if ( !this->isBlind() )
							{
								messagePlayer(player, language[599]);
							}
						}
						break;
					case EFF_BLIND:
						if ( !this->isBlind() )
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
						if ( levitation_hijacked )
						{
							bool sustained = false;
							Entity* caster = uidToEntity(levitation_hijacked->caster);
							if ( caster )
							{
								//Deduct mana from caster. Cancel spell if not enough mana (simply leave sustained at false).
								bool deducted = caster->safeConsumeMP(1); //Consume 1 mana ever duration / mana seconds
								if ( deducted )
								{
									sustained = true;
									myStats->EFFECTS[c] = true;
									myStats->EFFECTS_TIMERS[c] = levitation_hijacked->channel_duration;
								}
								else
								{
									int i = 0;
									for ( i = 0; i < 4; ++i )
									{
										if ( players[i]->entity == caster )
										{
											messagePlayer(i, language[606]);    //TODO: Unhardcode name?
										}
									}
									list_RemoveNode(levitation_hijacked->magic_effects_node); //Remove it from the entity's magic effects. This has the side effect of removing it from the sustained spells list too.
								}
							}
							if ( sustained )
							{
								dissipate = false;    //Sustained the spell, so do not stop levitating.
							}
						}
						if ( dissipate )
						{
							messagePlayer(player, language[607]);
						}
						break;
					case EFF_TELEPATH:
						if ( myStats->mask != nullptr && myStats->mask->type == TOOL_BLINDFOLD_TELEPATHY )
						{
							// don't play any messages since we'll reset the counter in due time.
							// likely to happen on level change.
						}
						else
						{
							setEffect(EFF_TELEPATH, false, 0, true);
							messagePlayer(player, language[608]);
							if ( player == clientnum )
							{
								for ( node_t* mapNode = map.creatures->first; mapNode != nullptr; mapNode = mapNode->next )
								{
									Entity* mapCreature = (Entity*)mapNode->element;
									if ( mapCreature )
									{
										// undo telepath rendering.
										mapCreature->monsterEntityRenderAsTelepath = 0;
									}
								}
							}
						}
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
					case EFF_MAGICRESIST:
						messagePlayer(player, language[2470]);
						break;
					case EFF_MAGICREFLECT:
						dissipate = true; //Remove the effect by default.
						if ( reflectMagic_hijacked )
						{
							bool sustained = false;
							Entity* caster = uidToEntity(reflectMagic_hijacked->caster);
							if ( caster )
							{
								//Deduct mana from caster. Cancel spell if not enough mana (simply leave sustained at false).
								bool deducted = caster->safeConsumeMP(1); //Consume 1 mana ever duration / mana seconds
								if ( deducted )
								{
									sustained = true;
									myStats->EFFECTS[c] = true;
									myStats->EFFECTS_TIMERS[c] = reflectMagic_hijacked->channel_duration;
								}
								else
								{
									int i = 0;
									for ( i = 0; i < 4; ++i )
									{
										if ( players[i]->entity == caster )
										{
											messagePlayer(i, language[2474]);
										}
									}
									list_RemoveNode(reflectMagic_hijacked->magic_effects_node); //Remove it from the entity's magic effects. This has the side effect of removing it from the sustained spells list too.
																								//list_RemoveNode(reflectMagic_hijacked->sustain_node); //Remove it from the channeled spells list.
								}
							}
							if ( sustained )
							{
								dissipate = false; //Sustained the spell, so do not stop being invisible.
							}
						}
						if ( dissipate )
						{
							messagePlayer(player, language[2471]);
							updateClient = true;
						}
						break;
					case EFF_VAMPIRICAURA:
						dissipate = true; //Remove the effect by default.
						if ( vampiricAura_hijacked )
						{
							bool sustained = false;
							Entity* caster = uidToEntity(vampiricAura_hijacked->caster);
							if ( caster )
							{
								//Deduct mana from caster. Cancel spell if not enough mana (simply leave sustained at false).
								bool deducted = caster->safeConsumeMP(3); //Consume 1 mana ever duration / mana seconds
								if ( deducted )
								{
									sustained = true;
									myStats->EFFECTS[c] = true;
									myStats->EFFECTS_TIMERS[c] = vampiricAura_hijacked->channel_duration;

									// monsters have a chance to un-sustain the spell each MP consume.
									if ( caster->behavior == &actMonster && rand() % 20 == 0 )
									{
										sustained = false;
										list_RemoveNode(vampiricAura_hijacked->magic_effects_node);
									}
								}
								else
								{
									int i = 0;
									for ( i = 0; i < 4; ++i )
									{
										if ( players[i]->entity == caster )
										{
											//messagePlayer(player, language[2449]);
										}
									}
									list_RemoveNode(vampiricAura_hijacked->magic_effects_node); //Remove it from the entity's magic effects. This has the side effect of removing it from the sustained spells list too.
																								//list_RemoveNode(reflectMagic_hijacked->sustain_node); //Remove it from the channeled spells list.
								}
							}
							if ( sustained )
							{
								dissipate = false; //Sustained the spell, so do not stop being invisible.
							}
						}
						if ( dissipate )
						{
							if ( myStats->HUNGER > 250 )
							{
								myStats->HUNGER = 252; // set to above 250 to trigger the hunger sound/messages when it decrements to 250.
								serverUpdateHunger(player);
							}
							messagePlayer(player, language[2449]);
							updateClient = true;
						}
						break;
					case EFF_SLOW:
						messagePlayer(player, language[604]); // "You return to your normal speed."
						break;
					case EFF_POLYMORPH:
						effectPolymorph = 0;
						serverUpdateEntitySkill(this, 50);
						messagePlayer(player, language[3185]);

						playSoundEntity(this, 400, 92);
						createParticleDropRising(this, 593, 1.f);
						serverSpawnMiscParticles(this, PARTICLE_EFFECT_RISING_DROP, 593);
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

	if ( updateClient )
	{
		//Only a select few effects have something that needs to be handled on the client's end.
		//(such as spawning particles for the magic reflection effect)
		//Only update the entity's effects in that case.
		serverUpdateEffectsForEntity(true);
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

		if ( skill == PRO_SPELLCASTING && skillCapstoneUnlockedEntity(PRO_SPELLCASTING) )
		{
			//Spellcasting capstone = free casting of Forcebolt.
			//Give the player the spell if they haven't learned it yet.
			if ( player > 0 && multiplayer == SERVER )
			{
				strcpy((char*)net_packet->data, "ASPL");
				net_packet->data[4] = clientnum;
				net_packet->data[5] = SPELL_FORCEBOLT;
				net_packet->address.host = net_clients[player - 1].host;
				net_packet->address.port = net_clients[player - 1].port;
				net_packet->len = 6;
				sendPacketSafe(net_sock, -1, net_packet, player - 1);
			}
			else if ( player >= 0 )
			{
				addSpell(SPELL_FORCEBOLT, player, true);
			}
		}

		if ( skill == PRO_SWIMMING && !(svFlags & SV_FLAG_HUNGER) )
		{
			// hunger off and swimming is raised.
			serverUpdatePlayerGameplayStats(player, STATISTICS_HOT_TUB_TIME_MACHINE, 1);
		}

		if ( skill == PRO_MAGIC && skillCapstoneUnlockedEntity(PRO_MAGIC) )
		{
			//magic capstone = bonus spell: Dominate.
			if ( player > 0 && multiplayer == SERVER )
			{
				strcpy((char*)net_packet->data, "ASPL");
				net_packet->data[4] = clientnum;
				net_packet->data[5] = SPELL_DOMINATE;
				net_packet->address.host = net_clients[player - 1].host;
				net_packet->address.port = net_clients[player - 1].port;
				net_packet->len = 6;
				sendPacketSafe(net_sock, -1, net_packet, player - 1);
			}
			else if ( player >= 0 )
			{
				addSpell(SPELL_DOMINATE, player, true);
			}
		}
		myStats->EXP += 2;
	}

	int statBonusSkill = getStatForProficiency(skill);

	if ( statBonusSkill >= STAT_STR )
	{
		// stat has chance for bonus point if the relevant proficiency has been trained.
		// write the last proficiency that effected the skill.
		myStats->PLAYER_LVL_STAT_BONUS[statBonusSkill] = skill;
	}

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

Stat* Entity::getStats() const
{
	if ( this->behavior == &actMonster ) // monsters
	{
		if ( multiplayer == CLIENT && clientStats )
		{
			return clientStats;
		}
		if ( this->children.first != nullptr )
		{
			if ( this->children.first->next != nullptr )
			{
				return (Stat*)this->children.first->next->element;
			}
		}
	}
	else if ( this->behavior == &actPlayer ) // players
	{
		return stats[this->skill[2]];
	}
	else if ( this->behavior == &actPlayerLimb ) // player bodyparts
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
	if ( !myStats )
	{
		return; //Can't continue without these.
	}

	list_t* items = nullptr;
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
	Item* oldarmor = nullptr;

	node_t* node = nullptr;

	bool glovesandshoes = false;
	if ( myStats->type == HUMAN )
	{
		glovesandshoes = true;
	}

	if ( items )
	{
		/*
		* Rundown of the function:
		* Loop through all items.
		* Check the monster's item. Compare and grab the best item.
		*/

		for ( node = items->first; node != nullptr; node = node->next )
		{
			//Turn the entity into an item.
			if ( node->element )
			{
				Entity* entity = (Entity*)node->element;
				Item* item = nullptr;
				if ( entity != nullptr )
				{
					item = newItemFromEntity(entity);
				}
				if ( !item )
				{
					continue;
				}
				if ( !canWieldItem(*item) )
				{
					free(item);
					continue;
				}

				//If weapon.
				if ( itemCategory(item) == WEAPON )
				{
					if ( myStats->weapon == nullptr ) //Not currently holding a weapon.
					{
						myStats->weapon = item; //Assign the monster's weapon.
						item = nullptr;
						list_RemoveNode(entity->mynode);
					}
					else
					{
						//Ok, the monster has a weapon already. First check if the monster's weapon is cursed. Can't drop it if it is.
						if ( myStats->weapon->beatitude >= 0 && itemCategory(myStats->weapon) != MAGICSTAFF && itemCategory(myStats->weapon) != POTION && itemCategory(myStats->weapon) != THROWN && itemCategory(myStats->weapon) != GEM )
						{
							//Next compare the two weapons. If the item on the ground is better, drop the weapon it's carrying and equip that one.
							int weapon_tohit = myStats->weapon->weaponGetAttack();
							int new_weapon_tohit = item->weaponGetAttack();

							//If the new weapon does more damage than the current weapon.
							if ( new_weapon_tohit > weapon_tohit )
							{
								dropItemMonster(myStats->weapon, this, myStats);
								myStats->weapon = item;
								item = nullptr;
								list_RemoveNode(entity->mynode);
							}
						}
					}
				}
				else if ( itemCategory(item) == ARMOR )
				{
					if ( checkEquipType(item) == TYPE_HAT ) // hats
					{
						if ( myStats->helmet == nullptr ) // nothing on head currently
						{
							// goblins love hats.
							myStats->helmet = item; // pick up the hat.
							item = nullptr;
							list_RemoveNode(entity->mynode);
						}
					}
					else if ( checkEquipType(item) == TYPE_HELM ) // helmets
					{
						if ( myStats->helmet == nullptr ) // nothing on head currently
						{
							myStats->helmet = item; // pick up the helmet.
							item = nullptr;
							list_RemoveNode(entity->mynode);
						}
						else
						{
							if ( myStats->helmet->beatitude >= 0 ) // if the armor is not cursed, proceed. Won't do anything if the armor is cursed.
							{
								// to compare the armors, we use the AC function to check the Armor Class of the equipment the goblin
								// is currently wearing versus the Armor Class that the goblin would have if it had the new armor.
								currentAC = AC(myStats);
								oldarmor = myStats->helmet;
								myStats->helmet = item;
								newAC = AC(myStats);
								myStats->helmet = oldarmor;

								//If the new armor is better than the current armor.
								if ( newAC > currentAC )
								{
									dropItemMonster(myStats->helmet, this, myStats);
									myStats->helmet = item;
									item = nullptr;
									list_RemoveNode(entity->mynode);
								}
							}
						}
					}
					else if ( checkEquipType(item) == TYPE_SHIELD )     // shields
					{
						if ( myStats->shield == nullptr ) // nothing in left hand currently
						{
							myStats->shield = item; // pick up the shield.
							item = nullptr;
							list_RemoveNode(entity->mynode);
						}
						else
						{
							if ( myStats->shield->beatitude >= 0 )   // if the armor is not cursed, proceed. Won't do anything if the armor is cursed.
							{
								// to compare the armors, we use the AC function to check the Armor Class of the equipment the goblin
								// is currently wearing versus the Armor Class that the goblin would have if it had the new armor.
								currentAC = AC(myStats);
								oldarmor = myStats->shield;
								myStats->shield = item;
								newAC = AC(myStats);
								myStats->shield = oldarmor;

								//If the new armor is better than the current armor (OR we're not carrying anything)
								if ( newAC > currentAC || !myStats->shield )
								{
									dropItemMonster(myStats->shield, this, myStats);
									myStats->shield = item;
									item = nullptr;
									list_RemoveNode(entity->mynode);
								}
							}
						}
					}
					else if ( checkEquipType(item) == TYPE_BREASTPIECE ) // breastpieces
					{
						if ( myStats->breastplate == nullptr ) // nothing on torso currently
						{
							myStats->breastplate = item; // pick up the armor.
							item = nullptr;
							list_RemoveNode(entity->mynode);
						}
						else
						{
							if ( myStats->breastplate->beatitude >= 0 ) // if the armor is not cursed, proceed. Won't do anything if the armor is cursed.
							{
								// to compare the armors, we use the AC function to check the Armor Class of the equipment the goblin
								// is currently wearing versus the Armor Class that the goblin would have if it had the new armor.
								currentAC = AC(myStats);
								oldarmor = myStats->breastplate;
								myStats->breastplate = item;
								newAC = AC(myStats);
								myStats->breastplate = oldarmor;

								//If the new armor is better than the current armor.
								if ( newAC > currentAC )
								{
									dropItemMonster(myStats->breastplate, this, myStats);
									myStats->breastplate = item;
									item = nullptr;
									list_RemoveNode(entity->mynode);
								}
							}
						}
					}
					else if ( checkEquipType(item) == TYPE_CLOAK ) // cloaks
					{
						if ( myStats->cloak == nullptr ) // nothing on back currently
						{
							myStats->cloak = item; // pick up the armor.
							item = nullptr;
							list_RemoveNode(entity->mynode);
						}
						else
						{
							if ( myStats->cloak->beatitude >= 0 )   // if the armor is not cursed, proceed. Won't do anything if the armor is cursed.
							{
								// to compare the armors, we use the AC function to check the Armor Class of the equipment the goblin
								// is currently wearing versus the Armor Class that the goblin would have if it had the new armor.
								currentAC = AC(myStats);
								oldarmor = myStats->cloak;
								myStats->cloak = item;
								newAC = AC(myStats);
								myStats->cloak = oldarmor;

								//If the new armor is better than the current armor.
								if ( newAC > currentAC )
								{
									dropItemMonster(myStats->cloak, this, myStats);
									myStats->cloak = item;
									item = nullptr;
									list_RemoveNode(entity->mynode);
								}
							}
						}
					}
					if ( glovesandshoes && item != nullptr )
					{
						if ( checkEquipType(item) == TYPE_BOOTS ) // boots
						{
							if ( myStats->shoes == nullptr )
							{
								myStats->shoes = item; // pick up the armor
								item = nullptr;
								list_RemoveNode(entity->mynode);
							}
							else
							{
								if ( myStats->shoes->beatitude >= 0 ) // if the armor is not cursed, proceed. Won't do anything if the armor is cursed.
								{
									// to compare the armors, we use the AC function to check the Armor Class of the equipment the goblin
									// is currently wearing versus the Armor Class that the goblin would have if it had the new armor.
									currentAC = AC(myStats);
									oldarmor = myStats->shoes;
									myStats->shoes = item;
									newAC = AC(myStats);
									myStats->shoes = oldarmor;

									//If the new armor is better than the current armor.
									if ( newAC > currentAC )
									{
										dropItemMonster(myStats->shoes, this, myStats);
										myStats->shoes = item;
										item = nullptr;
										list_RemoveNode(entity->mynode);
									}
								}
							}
						}
						else if ( checkEquipType(item) == TYPE_GLOVES )
						{
							if ( myStats->gloves == nullptr )
							{
								myStats->gloves = item; // pick up the armor
								item = nullptr;
								list_RemoveNode(entity->mynode);
							}
							else
							{
								if ( myStats->gloves->beatitude >= 0 ) // if the armor is not cursed, proceed. Won't do anything if the armor is cursed.
								{
									// to compare the armors, we use the AC function to check the Armor Class of the equipment the goblin
									// is currently wearing versus the Armor Class that the goblin would have if it had the new armor.
									currentAC = AC(myStats);
									oldarmor = myStats->gloves;
									myStats->gloves = item;
									newAC = AC(myStats);
									myStats->gloves = oldarmor;

									//If the new armor is better than the current armor.
									if ( newAC > currentAC )
									{
										dropItemMonster(myStats->gloves, this, myStats);
										myStats->gloves = item;
										item = nullptr;
										list_RemoveNode(entity->mynode);
									}
								}
							}
						}
					}
				}
				else if ( itemCategory(item) == POTION )
				{
					if ( myStats->weapon == nullptr ) //Not currently holding a weapon.
					{
						myStats->weapon = item; //Assign the monster's weapon.
						item = nullptr;
						list_RemoveNode(entity->mynode);
					}
					//Don't pick up if already wielding something.
				}
				else if ( itemCategory(item) == THROWN )
				{
					if ( myStats->weapon == nullptr ) //Not currently holding a weapon.
					{
						if ( !entity->itemNotMoving && entity->parent && entity->parent != uid )
						{
							//Don't pick up the item.
						}
						else
						{
							myStats->weapon = item; //Assign the monster's weapon.
							item = nullptr;
							list_RemoveNode(entity->mynode);
						}
					}
					//Don't pick up if already wielding something.
				}

				if ( item != nullptr )
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
	if ( it != map.entities_map.end() )
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
	if ( !entitystats )
	{
		return;
	}

	int healthDiff = entitystats->HP;

	if ( this->behavior == &actPlayer && godmode )
	{
		amount = entitystats->MAXHP;
	}
	if ( !entitystats || amount == entitystats->HP )
	{
		return;
	}
	entitystats->HP = std::min(std::max(0, amount), entitystats->MAXHP);
	healthDiff -= entitystats->HP;
	strncpy(entitystats->obituary, language[1500], 127);

	if ( this->behavior == &actPlayer && buddhamode && entitystats->HP < 1 )
	{
		entitystats->HP = 1; //Buddhas never die!
	}

	int i = 0;
	if ( multiplayer == SERVER )
	{
		for ( i = 1; i < numplayers; i++ )
		{
			if ( this == players[i]->entity )
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
		if ( this->behavior == &actMonster )
		{
			if ( this->monsterAllyIndex >= 1 && this->monsterAllyIndex < MAXPLAYERS )
			{
				if ( abs(healthDiff) == 1 || healthDiff == 0 )
				{
					serverUpdateAllyHP(this->monsterAllyIndex, getUID(), entitystats->HP, entitystats->MAXHP, true);
				}
				else
				{
					serverUpdateAllyHP(this->monsterAllyIndex, getUID(), entitystats->HP, entitystats->MAXHP, true);
				}
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

	if ( this->behavior == &actPlayer && godmode )
	{
		amount = entitystats->MAXMP;
	}
	if ( !entitystats || amount == entitystats->MP )
	{
		return;
	}
	entitystats->MP = std::min(std::max(0, amount), entitystats->MAXMP);

	int i = 0;
	if ( multiplayer == SERVER )
	{
		for ( i = 1; i < numplayers; i++ )
		{
			if ( this == players[i]->entity )
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

	if ( !entitystats )
	{
		return;
	}

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

int Entity::getMP()
{
	Stat* myStats = getStats();

	if ( !myStats )
	{
		return 0;
	}

	return myStats->MP;
}

int Entity::getHP()
{
	Stat* myStats = getStats();

	if ( !myStats )
	{
		return 0;
	}

	return myStats->HP;
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
	if ( entitystats == NULL || amount == 0 )
	{
		return;
	}

	int overdrawn = 0;
	entitystats->MP -= amount;
	int player = -1;
	int i = 0;
	for ( i = 0; i < numplayers; ++i )
	{
		if ( this == players[i]->entity )
		{
			player = i; //Set the player.
		}
	}
	if ( entitystats->MP < 0 )
	{
		//Overdrew. Take that extra and flow it over into HP.
		overdrawn = entitystats->MP;
		entitystats->MP = 0;
	}
	if ( multiplayer == SERVER )
	{
		//First check if the entity is the player.
		for ( i = 1; i < numplayers; ++i )
		{
			if ( this == players[i]->entity )
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
	else if ( clientnum != 0 && multiplayer == CLIENT )
	{
		if ( this == players[clientnum]->entity )
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

	if ( overdrawn < 0 )
	{
		if ( player >= 0 )
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
	if ( !stat )
	{
		return false;
	}

	if ( amount > stat->MP )
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
	int increasestat[3] = { 0, 0, 0 };
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
		if ( !(behavior == &actMonster && monsterAllySummonRank != 0) )
		{
			myStats->MP += MP_MOD;
			myStats->MAXMP += MP_MOD;
			myStats->MP = std::min(myStats->MP, myStats->MAXMP);
		}

		// now pick three attributes to increase

		if ( player >= 0 )
		{
			// players only.
			playerStatIncrease(client_classes[player], increasestat);
		}
		else if ( behavior == &actMonster && monsterAllySummonRank != 0 )
		{
			bool secondSummon = false;
			if ( !strcmp(myStats->name, "skeleton knight") )
			{
				playerStatIncrease(1, increasestat); // warrior weighting
			}
			else if ( !strcmp(myStats->name, "skeleton sentinel") )
			{
				secondSummon = true;
				playerStatIncrease(5, increasestat); // rogue weighting
			}

			bool rankUp = false;

			if ( myStats->type == SKELETON )
			{
				int rank = myStats->LVL / 5;
				if ( rank <= 6 && myStats->LVL % 5 == 0 )
				{
					// went up a rank (every 5 LVLs)
					rank = std::min(1 + rank, 7);
					rankUp = true;
					createParticleDropRising(this, 791, 1.0);
					serverSpawnMiscParticles(this, PARTICLE_EFFECT_RISING_DROP, 791);
					skeletonSummonSetEquipment(myStats, std::min(7, 1 + (myStats->LVL / 5)));
				}
			}

			for ( i = 0; i < 3; i++ )
			{
				switch ( increasestat[i] )
				{
					case STAT_STR:
						myStats->STR++;
						break;
					case STAT_DEX:
						myStats->DEX++;
						break;
					case STAT_CON:
						myStats->CON++;
						break;
					case STAT_INT:
						myStats->INT++;
						break;
					case STAT_PER:
						myStats->PER++;
						break;
					case STAT_CHR:
						myStats->CHR++;
						break;
					default:
						break;
				}

			}
			Entity* leader = uidToEntity(myStats->leader_uid);
			if ( leader )
			{
				Stat* leaderStats = leader->getStats();
				if ( leaderStats )
				{
					if ( !secondSummon )
					{
						leaderStats->playerSummonLVLHP = (myStats->LVL << 16);
						leaderStats->playerSummonLVLHP |= (myStats->MAXHP);

						leaderStats->playerSummonSTRDEXCONINT = (myStats->STR << 24);
						leaderStats->playerSummonSTRDEXCONINT |= (myStats->DEX << 16);
						leaderStats->playerSummonSTRDEXCONINT |= (myStats->CON << 8);
						leaderStats->playerSummonSTRDEXCONINT |= (myStats->INT);

						leaderStats->playerSummonPERCHR = (myStats->PER << 24);
						leaderStats->playerSummonPERCHR |= (myStats->CHR << 16);
						leaderStats->playerSummonPERCHR |= (this->monsterAllySummonRank << 8);
					}
					else
					{
						leaderStats->playerSummon2LVLHP = (myStats->LVL << 16);
						leaderStats->playerSummon2LVLHP |= (myStats->MAXHP);

						leaderStats->playerSummon2STRDEXCONINT = (myStats->STR << 24);
						leaderStats->playerSummon2STRDEXCONINT |= (myStats->DEX << 16);
						leaderStats->playerSummon2STRDEXCONINT |= (myStats->CON << 8);
						leaderStats->playerSummon2STRDEXCONINT |= (myStats->INT);

						leaderStats->playerSummon2PERCHR = (myStats->PER << 24);
						leaderStats->playerSummon2PERCHR |= (myStats->CHR << 16);
						leaderStats->playerSummon2PERCHR |= (this->monsterAllySummonRank << 8);
					}
					if ( leader->behavior == &actPlayer )
					{
						serverUpdatePlayerSummonStrength(leader->skill[2]);
						if ( rankUp )
						{
							color = SDL_MapRGB(mainsurface->format, 255, 255, 0);
							messagePlayerMonsterEvent(leader->skill[2], color, *myStats, language[3197], language[3197], MSG_GENERIC);
							playSoundPlayer(leader->skill[2], 40, 64);
						}
					}
				}
			}
		}
		else
		{
			// monsters use this.
			increasestat[0] = rand() % 6;
			int r = rand() % 6;
			while ( r == increasestat[0] ) {
				r = rand() % 6;
			}
			increasestat[1] = r;
			r = rand() % 6;
			while ( r == increasestat[0] || r == increasestat[1] ) {
				r = rand() % 6;
			}
			increasestat[2] = r;

			for ( i = 0; i < 3; i++ )
			{
				switch ( increasestat[i] )
				{
					case STAT_STR:
						myStats->STR++;
						break;
					case STAT_DEX:
						myStats->DEX++;
						break;
					case STAT_CON:
						myStats->CON++;
						break;
					case STAT_INT:
						myStats->INT++;
						break;
					case STAT_PER:
						myStats->PER++;
						break;
					case STAT_CHR:
						myStats->CHR++;
						break;
				}
			}
		}

		if ( behavior == &actMonster )
		{
			if ( myStats->leader_uid )
			{
				Entity* leader = uidToEntity(myStats->leader_uid);
				if ( leader )
				{
					for ( i = 0; i < MAXPLAYERS; ++i )
					{
						if ( players[i] && players[i]->entity == leader )
						{
							color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
							messagePlayerMonsterEvent(i, color, *myStats, language[2379], language[2379], MSG_GENERIC);
							playSoundEntity(this, 97, 128);
							serverUpdateAllyStat(i, getUID(), myStats->LVL, myStats->HP, myStats->MAXHP, myStats->type);
						}
					}
				}
			}
		}

		if ( player >= 0 )
		{
			for ( i = 0; i < NUMSTATS * 2; ++i )
			{
				myStats->PLAYER_LVL_STAT_TIMER[i] = 0;
			}

			bool rolledBonusStat = false;
			int statIconTicks = 250;

			for ( i = 0; i < 3; i++ )
			{
				messagePlayerColor(player, color, language[623 + increasestat[i]]);
				switch ( increasestat[i] )
				{
					case STAT_STR: // STR
						myStats->STR++;
						myStats->PLAYER_LVL_STAT_TIMER[increasestat[i]] = statIconTicks;
						if ( myStats->PLAYER_LVL_STAT_BONUS[increasestat[i]] >= PRO_LOCKPICKING && !rolledBonusStat )
						{
							if ( rand() % 5 == 0 )
							{
								myStats->STR++;
								rolledBonusStat = true;
								myStats->PLAYER_LVL_STAT_TIMER[increasestat[i] + NUMSTATS] = statIconTicks;
								//messagePlayer(0, "Rolled bonus in %d", increasestat[i]);
							}
						}
						break;
					case STAT_DEX: // DEX
						myStats->DEX++;
						myStats->PLAYER_LVL_STAT_TIMER[increasestat[i]] = statIconTicks;
						if ( myStats->PLAYER_LVL_STAT_BONUS[increasestat[i]] >= PRO_LOCKPICKING && !rolledBonusStat )
						{
							if ( rand() % 5 == 0 )
							{
								myStats->DEX++;
								rolledBonusStat = true;
								myStats->PLAYER_LVL_STAT_TIMER[increasestat[i] + NUMSTATS] = statIconTicks;
								//messagePlayer(0, "Rolled bonus in %d", increasestat[i]);
							}
						}
						break;
					case STAT_CON: // CON
						myStats->CON++;
						myStats->PLAYER_LVL_STAT_TIMER[increasestat[i]] = statIconTicks;
						if ( myStats->PLAYER_LVL_STAT_BONUS[increasestat[i]] >= PRO_LOCKPICKING && !rolledBonusStat )
						{
							if ( rand() % 5 == 0 )
							{
								myStats->CON++;
								rolledBonusStat = true;
								myStats->PLAYER_LVL_STAT_TIMER[increasestat[i] + NUMSTATS] = statIconTicks;
								//messagePlayer(0, "Rolled bonus in %d", increasestat[i]);
							}
						}
						break;
					case STAT_INT: // INT
						myStats->INT++;
						myStats->PLAYER_LVL_STAT_TIMER[increasestat[i]] = statIconTicks;
						if ( myStats->PLAYER_LVL_STAT_BONUS[increasestat[i]] >= PRO_LOCKPICKING && !rolledBonusStat )
						{
							if ( rand() % 5 == 0 )
							{
								myStats->INT++;
								rolledBonusStat = true;
								myStats->PLAYER_LVL_STAT_TIMER[increasestat[i] + NUMSTATS] = statIconTicks;
								//messagePlayer(0, "Rolled bonus in %d", increasestat[i]);
							}
						}
						break;
					case STAT_PER: // PER
						myStats->PER++;
						myStats->PLAYER_LVL_STAT_TIMER[increasestat[i]] = statIconTicks;
						if ( myStats->PLAYER_LVL_STAT_BONUS[increasestat[i]] >= PRO_LOCKPICKING && !rolledBonusStat )
						{
							if ( rand() % 5 == 0 )
							{
								myStats->PER++;
								rolledBonusStat = true;
								myStats->PLAYER_LVL_STAT_TIMER[increasestat[i] + NUMSTATS] = statIconTicks;
								//messagePlayer(0, "Rolled bonus in %d", increasestat[i]);
							}
						}
						break;
					case STAT_CHR: // CHR
						myStats->CHR++;
						myStats->PLAYER_LVL_STAT_TIMER[increasestat[i]] = statIconTicks;
						if ( myStats->PLAYER_LVL_STAT_BONUS[increasestat[i]] >= PRO_LOCKPICKING && !rolledBonusStat )
						{
							if ( rand() % 5 == 0 )
							{
								myStats->CHR++;
								rolledBonusStat = true;
								myStats->PLAYER_LVL_STAT_TIMER[increasestat[i] + NUMSTATS] = statIconTicks;
								//messagePlayer(0, "Rolled bonus in %d", increasestat[i]);
							}
						}
						break;
				}
			}

			for ( i = 0; i < MAXPLAYERS; ++i )
			{
				// broadcast a player levelled up to other players.
				if ( i != player )
				{
					if ( client_disconnected[i] )
					{
						continue;
					}
					messagePlayerMonsterEvent(i, color, *myStats, language[2379], language[2379], MSG_GENERIC);
				}
			}
		}

		// inform clients of stat changes
		if ( multiplayer == SERVER )
		{
			if ( player > 0 )
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

				strcpy((char*)net_packet->data, "LVLI");
				net_packet->data[4] = clientnum;
				net_packet->data[5] = (Uint8)myStats->PLAYER_LVL_STAT_TIMER[STAT_STR];
				net_packet->data[6] = (Uint8)myStats->PLAYER_LVL_STAT_TIMER[STAT_DEX];
				net_packet->data[7] = (Uint8)myStats->PLAYER_LVL_STAT_TIMER[STAT_CON];
				net_packet->data[8] = (Uint8)myStats->PLAYER_LVL_STAT_TIMER[STAT_INT];
				net_packet->data[9] = (Uint8)myStats->PLAYER_LVL_STAT_TIMER[STAT_PER];
				net_packet->data[10] = (Uint8)myStats->PLAYER_LVL_STAT_TIMER[STAT_CHR];
				net_packet->data[11] = (Uint8)myStats->PLAYER_LVL_STAT_TIMER[STAT_STR + NUMSTATS];
				net_packet->data[12] = (Uint8)myStats->PLAYER_LVL_STAT_TIMER[STAT_DEX + NUMSTATS];
				net_packet->data[13] = (Uint8)myStats->PLAYER_LVL_STAT_TIMER[STAT_CON + NUMSTATS];
				net_packet->data[14] = (Uint8)myStats->PLAYER_LVL_STAT_TIMER[STAT_INT + NUMSTATS];
				net_packet->data[15] = (Uint8)myStats->PLAYER_LVL_STAT_TIMER[STAT_PER + NUMSTATS];
				net_packet->data[16] = (Uint8)myStats->PLAYER_LVL_STAT_TIMER[STAT_CHR + NUMSTATS];
				net_packet->address.host = net_clients[player - 1].host;
				net_packet->address.port = net_clients[player - 1].port;
				net_packet->len = 17;
				sendPacketSafe(net_sock, -1, net_packet, player - 1);
			}
			serverUpdatePlayerLVL(); // update all clients of party levels.
		}

		for ( i = 0; i < NUMSTATS; ++i )
		{
			myStats->PLAYER_LVL_STAT_BONUS[i] = -1;
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
	bool vampiricHunger = false;
	if ( myStats->EFFECTS[EFF_VAMPIRICAURA] )
	{
		vampiricHunger = true;
	}

	if ( !strncmp(map.name, "Sanctum", 7) 
		|| !strncmp(map.name, "Boss", 4) 
		|| !strncmp(map.name, "Hell Boss", 4)
		|| !strncmp(map.name, "Hamlet", 6) )
	{
		hungerring = 1; // slow down hunger on boss stages.
	}

	int hungerTickRate = 30; // how many ticks to reduce hunger by a point.
	if ( vampiricHunger )
	{
		hungerTickRate = 5;
	}
	else if ( hungerring > 0 )
	{
		hungerTickRate = 120;
	}
	else if ( hungerring < 0 )
	{
		hungerTickRate = 15;
	}

	int playerCount = 0;
	for ( i = 0; i < MAXPLAYERS; ++i )
	{
		if ( !client_disconnected[i] )
		{
			++playerCount;
		}
	}

	if ( !(svFlags & SV_FLAG_HARDCORE) )
	{
		if ( playerCount == 3 )
		{
			hungerTickRate *= 1.25;
		}
		else if ( playerCount == 4 )
		{
			hungerTickRate *= 1.5;
		}
	}

	bool processHunger = (svFlags & SV_FLAG_HUNGER); // check server flags if hunger is enabled.
	if ( player >= 0 )
	{
		if ( myStats->type == SKELETON || myStats->type == AUTOMATON )
		{
			processHunger = false;
		}
	}

	if ( !processHunger )
	{
		if ( myStats->HUNGER != 800 )
		{
			myStats->HUNGER = 800; // always set hunger to 800
			serverUpdateHunger(player);
		}
	}
	else if ( ticks % hungerTickRate == 0 )
	{
		//messagePlayer(0, "hungertick %d, curr %d, players: %d", hungerTickRate, myStats->HUNGER, playerCount);
		if ( myStats->HUNGER > 0 )
		{
			myStats->HUNGER--;
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
			// Process HUNGER Effect - Wasting Away
			myStats->HUNGER = 0;

			// Deal Hunger damage every three seconds
			if ( !myStats->EFFECTS[EFF_VOMITING] && ticks % 150 == 0 )
			{
				serverUpdateHunger(player);

				if ( player >= 0 ) // Only Players can starve
				{
					if ( buddhamode )
					{
						if ( myStats->HP - 4 > 0 )
						{
							this->modHP(-4);
						}
						else
						{
							// Instead of killing the Buddha Player, set their HP to 1
							this->setHP(1);
						}
					}
					else
					{
						this->modHP(-4);

						if ( myStats->HP <= 0 )
						{
							this->setObituary(language[1530]);
						}
					}

					// Give the Player feedback on being hurt
					playSoundEntity(this, 28, 64); // "Damage.ogg"

					if ( myStats->HP > 0 )
					{
						messagePlayer(player, language[633]);
					}

					// Shake the Host's screen
					if ( player == clientnum )
					{
						camera_shakex += .1;
						camera_shakey += 10;
					}
					else if ( player > 0 && multiplayer == SERVER )
					{
						// Shake the Client's screen
						strcpy((char*)net_packet->data, "SHAK");
						net_packet->data[4] = 10; // turns into .1
						net_packet->data[5] = 10;
						net_packet->address.host = net_clients[player - 1].host;
						net_packet->address.port = net_clients[player - 1].port;
						net_packet->len = 6;
						sendPacketSafe(net_sock, -1, net_packet, player - 1);
					}
				}
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
			serverUpdatePlayerGameplayStats(player, STATISTICS_TEMPT_FATE, 5);
		}
	}

	// vomiting
	if ( myStats->EFFECTS[EFF_VOMITING] && ticks % 2 == 0 )
	{
		Entity* entity = spawnGib(this);
		if ( entity )
		{
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
	}

	// healing over time
	int healring = 0;
	int healthRegenInterval = getHealthRegenInterval(*myStats);
	bool naturalHeal = false;
	if ( healthRegenInterval >= 0 )
	{
		if ( myStats->HP < myStats->MAXHP )
		{
			this->char_heal++;
			if ( healring > 0 || (svFlags & SV_FLAG_HUNGER) || behavior == &actMonster )
			{
				if ( this->char_heal >= healthRegenInterval )
				{
					this->char_heal = 0;
					this->modHP(1);
					naturalHeal = true;
				}
			}
		}
		else
		{
			this->char_heal = 0;
		}
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
	int manaRegenInterval = getManaRegenInterval(*myStats);

	if ( myStats->MP < myStats->MAXMP && this->monsterAllySummonRank == 0 )
	{
		this->char_energize++;
		if ( this->char_energize >= manaRegenInterval )
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
		if ( myStats->weapon != NULL && myStats->weapon->beatitude >= 0 )
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
			if ( (this->char_torchtime >= 7200 && myStats->shield->type == TOOL_TORCH) || (this->char_torchtime >= 10260) )
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
		if ( this->char_poison > 150 )   // three seconds
		{
			this->char_poison = 0;
			int poisonhurt = std::max(3, (myStats->MAXHP / 20));
			if ( myStats->type == LICH_ICE
				|| myStats->type == LICH_FIRE
				|| myStats->type == LICH
				|| myStats->type == DEVIL )
			{
				poisonhurt = std::min(poisonhurt, 15); // prevent doing 50+ dmg
			}
			if ( poisonhurt > 3 )
			{
				poisonhurt -= rand() % (std::max(1, poisonhurt / 4));
			}
			this->modHP(-poisonhurt);
			if ( myStats->HP <= 0 )
			{
				Entity* killer = uidToEntity(myStats->poisonKiller);
				if ( killer )
				{
					killer->awardXP(this, true, true);
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
			if ( myStats->HP > 5 + (std::max(0, getCON())) ) // CON increases when bleeding stops.
			{
				int bleedhurt = 1 + myStats->MAXHP / 30;
				if ( bleedhurt > 1 )
				{
					bleedhurt -= rand() % (std::max(1, bleedhurt / 2));
				}
				if ( getCON() > 0 )
				{
					bleedhurt -= (getCON() / 5);
				}
				if ( myStats->type == LICH_ICE
					|| myStats->type == LICH_FIRE
					|| myStats->type == LICH
					|| myStats->type == DEVIL )
				{
					bleedhurt = std::min(bleedhurt, 15); // prevent doing 50+ dmg
				}
				bleedhurt = std::max(1, bleedhurt);
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
					Entity* entity = nullptr;
					if ( gibtype[myStats->type] == 1 )
					{
						entity = newEntity(203, 1, map.entities, nullptr); //Blood entity.
					}
					else if ( gibtype[myStats->type] == 2 )
					{
						entity = newEntity(213, 1, map.entities, nullptr); //Blood entity.
					}
					else if ( gibtype[myStats->type] == 4 )
					{
						entity = newEntity(682, 1, map.entities, nullptr); //Blood entity.
					}
					if ( entity != NULL )
					{
						entity->x = this->x;
						entity->y = this->y;
						entity->z = 8.0 + (rand() % 20) / 100.0;
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

	if ( player >= 0 && myStats->EFFECTS[EFF_LEVITATING] && MFLAG_DISABLELEVITATION)
	{
		Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 255);
		messagePlayerColor(player, color, language[2382]); // disabled levitation.
		myStats->EFFECTS[EFF_LEVITATING] = false;
		myStats->EFFECTS_TIMERS[EFF_LEVITATING] = 0;
	}

	if ( myStats->EFFECTS[EFF_MAGICREFLECT] )
	{
		spawnAmbientParticles(80, 579, 10 + rand() % 40, 1.0, false);
	}

	if (myStats->EFFECTS[EFF_VAMPIRICAURA])
	{
		spawnAmbientParticles(40, 600, 20 + rand() % 30, 0.5, true);
	}

	if ( myStats->EFFECTS[EFF_PACIFY] )
	{
		if ( ticks % 25 == 0 || ticks % 40 == 0 )
		{
			spawnAmbientParticles(1, 685, 20 + rand() % 10, 0.5, true);
		}
	}

	if ( myStats->EFFECTS[EFF_POLYMORPH] )
	{
		if ( ticks % 25 == 0 || ticks % 40 == 0 )
		{
			spawnAmbientParticles(1, 593, 20 + rand() % 10, 0.5, true);
		}
	}

	if ( myStats->EFFECTS[EFF_INVISIBLE] && myStats->type == SHADOW )
	{
		spawnAmbientParticles(20, 175, 20 + rand() % 30, 0.5, true);
	}

	// Process Burning Status Effect
	if ( this->flags[BURNING] )
	{
		this->char_fire--; // Decrease the fire counter
		
		// Check to see if time has run out
		if ( this->char_fire <= 0 )
		{
			this->flags[BURNING] = false;
			messagePlayer(player, language[647]); // "The flames go out."
			serverUpdateEntityFlag(this, BURNING);
		}
		else
		{
			// If 0.6 seconds have passed (30 ticks), process the Burning Status Effect
			if ( (this->char_fire % TICKS_TO_PROCESS_FIRE) == 0 )
			{
				// Buddha should not die to fire
				if ( buddhamode )
				{
					Sint32 fireDamage = (-2 - rand() % 3); // Deal between -2 to -5 damage

					// Fire damage is negative, so it needs to be added
					if ( myStats->HP + fireDamage > 0 )
					{
						this->modHP(fireDamage);
					}
					else
					{
						this->setHP(1); // Instead of killing the Buddha Player, set their HP to 1
					}
				}
				else
				{
					// Player is not Buddha, process fire damage normally
					this->modHP(-2 - rand() % 3); // Deal between -2 to -5 damage

					// If the Entity died, handle experience
					if ( myStats->HP <= 0 )
					{
						this->setObituary(language[1533]); // "burns to a crisp."

						Entity* killer = uidToEntity(myStats->poisonKiller);
						if ( killer != nullptr )
						{
							killer->awardXP(this, true, true);
						}
					}
				}

				// Give the Player feedback on being hurt
				messagePlayer(player, language[644]); // "It burns! It burns!"
				playSoundEntity(this, 28, 64); // "Damage.ogg"

				// Shake the Camera
				if ( player == clientnum )
				{
					camera_shakey += 5;
				}
				else if ( player > 0 && multiplayer == SERVER )
				{
					strcpy((char*)net_packet->data, "SHAK");
					net_packet->data[4] = 0; // turns into 0
					net_packet->data[5] = 5;
					net_packet->address.host = net_clients[player - 1].host;
					net_packet->address.port = net_clients[player - 1].port;
					net_packet->len = 6;
					sendPacketSafe(net_sock, -1, net_packet, player - 1);
				}

				// If the Entity has a Cloak, process dealing damage to the Entity's Cloak
				if ( myStats->cloak != nullptr )
				{
					// 1 in 10 chance of dealing damage to Entity's cloak
					if ( rand() % 10 == 0 && myStats->cloak->type != ARTIFACT_CLOAK )
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
							messagePlayer(player, language[645], myStats->cloak->getName()); // "Your %s smoulders!"
						}
						else
						{
							messagePlayer(player, language[646], myStats->cloak->getName()); // "Your %s burns to ash!"
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
				
				// Check to see if the fire is put out
				if ( (rand() % this->chanceToPutOutFire) == 0 )
				{
					this->flags[BURNING] = false;
					messagePlayer(player, language[647]); // "The flames go out."
					serverUpdateEntityFlag(this, BURNING);
				}
			}
		}
	}
	else
	{
		this->char_fire = 0; // If not on fire, then reset fire counter TODOR: This seems unecessary, but is what poison does, this is happening every tick
	}

	if ( player >= 0 && stats[player]->type == SKELETON )
	{
		// life saving
		if ( myStats->HP <= 0 )
		{
			int spellCost = getCostOfSpell(&spell_summon, this);
			int numSummonedAllies = 0;
			int firstManaToRefund = 0;
			int secondManaToRefund = 0;
			for ( node_t* node = myStats->FOLLOWERS.first; node != nullptr; node = node->next )
			{
				Uint32* c = (Uint32*)node->element;
				Entity* mySummon = uidToEntity(*c);
				if ( mySummon && mySummon->monsterAllySummonRank != 0 )
				{
					Stat* mySummonStats = mySummon->getStats();
					if ( mySummonStats )
					{
						if ( numSummonedAllies == 0 )
						{
							mySummon->setMP(mySummonStats->MAXMP * (mySummonStats->HP / static_cast<float>(mySummonStats->MAXHP)));
							firstManaToRefund += std::min(spellCost, static_cast<int>((mySummonStats->MP / static_cast<float>(mySummonStats->MAXMP)) * spellCost)); // MP to restore
							mySummon->setHP(0); // sacrifice!
							++numSummonedAllies;
						}
						else if ( numSummonedAllies == 1 )
						{
							mySummon->setMP(mySummonStats->MAXMP * (mySummonStats->HP / static_cast<float>(mySummonStats->MAXHP)));
							secondManaToRefund += std::min(spellCost, static_cast<int>((mySummonStats->MP / static_cast<float>(mySummonStats->MAXMP)) * spellCost)); // MP to restore
							mySummon->setHP(0); // for glorious leader!
							++numSummonedAllies;
							break;
						}
					}
				}
			}

			if ( numSummonedAllies == 2 )
			{
				firstManaToRefund /= 2;
				secondManaToRefund /= 2;
			}
			bool revivedWithFriendship = false;
			if ( myStats->MP < 75 && numSummonedAllies > 0 )
			{
				revivedWithFriendship = true;
			}

			int manaTotal = myStats->MP + firstManaToRefund + secondManaToRefund;
			
			if ( manaTotal >= 75 )
			{
				messagePlayer(player, language[651]);
				if ( revivedWithFriendship )
				{
					messagePlayer(player, language[3198]);
				}
				else
				{
					messagePlayer(player, language[3180]);
				}
				messagePlayer(player, language[654]);

				playSoundEntity(this, 167, 128);
				createParticleDropRising(this, 174, 1.0);
				serverSpawnMiscParticles(this, PARTICLE_EFFECT_RISING_DROP, 174);
				// convert MP to HP
				manaTotal = myStats->MP;
				if ( safeConsumeMP(myStats->MP) )
				{
					this->setHP(std::min(manaTotal, myStats->MAXHP));
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
				for ( c = 0; c < NUMEFFECTS; c++ )
				{
					myStats->EFFECTS[c] = false;
					myStats->EFFECTS_TIMERS[c] = 0;
				}

				myStats->EFFECTS[EFF_LEVITATING] = true;
				myStats->EFFECTS_TIMERS[EFF_LEVITATING] = 5 * TICKS_PER_SECOND;

				this->flags[BURNING] = false;
				serverUpdateEntityFlag(this, BURNING);
				serverUpdateEffects(player);
			}
			else
			{
				messagePlayer(player, language[3181]);
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
		if ( myStats->amulet->type == AMULET_LIFESAVING )   //Fixed! (saves against boulder traps.) 
		{
			if ( myStats->HP <= 0 )
			{
				if ( myStats->HUNGER > 0 )
				{
					messagePlayer(player, language[651]);
				}
				if ( !this->isBlind() )
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

					playSoundEntity(this, 167, 128);
					createParticleDropRising(this, 174, 1.0);
					serverSpawnMiscParticles(this, PARTICLE_EFFECT_RISING_DROP, 174);

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
					
					// check if hovering over a pit
					//if ( !isLevitating(myStats) )
					//{
					//	int my_x, my_y, u, v;
					//	my_x = std::min(std::max<unsigned int>(1, this->x / 16), map.width - 2);
					//	my_y = std::min(std::max<unsigned int>(1, this->y / 16), map.height - 2);
					//	for ( u = my_x - 1; u <= my_x + 1; u++ )
					//	{
					//		for ( v = my_y - 1; v <= my_y + 1; v++ )
					//		{
					//			if ( entityInsideTile(this, u, v, 0) )   // no floor
					//			{
					//				break;
					//			}
					//		}
					//	}
					//}
					myStats->EFFECTS[EFF_LEVITATING] = true;
					myStats->EFFECTS_TIMERS[EFF_LEVITATING] = 5 * TICKS_PER_SECOND;

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

	if ( player >= 0 
		&& myStats->mask != nullptr
		&& myStats->mask->type == TOOL_BLINDFOLD_TELEPATHY
		&& (ticks % 65 == 0 || !myStats->EFFECTS[EFF_TELEPATH]) )
	{
		setEffect(EFF_TELEPATH, true, 100, true);
	}

	if ( player >= 0
		&& myStats->mask != nullptr
		&& (myStats->mask->type == TOOL_BLINDFOLD || myStats->mask->type == TOOL_BLINDFOLD_FOCUS || myStats->mask->type == TOOL_BLINDFOLD_TELEPATHY )
		&& (ticks % 65 == 0 || !myStats->EFFECTS[EFF_BLIND]) )
	{
		setEffect(EFF_BLIND, true, 100, true);
		if ( myStats->mask->type == TOOL_BLINDFOLD_FOCUS )
		{
			bool cured = false;
			if ( myStats->EFFECTS_TIMERS[EFF_ASLEEP] > 0 )
			{
				cured = true;
				myStats->EFFECTS_TIMERS[EFF_ASLEEP] = 1; // tick over to 0 and dissipate on the next check, and play the appropriate message.
			}
			if ( myStats->EFFECTS_TIMERS[EFF_PARALYZED] > 0 )
			{
				cured = true;
				myStats->EFFECTS_TIMERS[EFF_PARALYZED] = 1; // tick over to 0 and dissipate on the next check, and play the appropriate message.
			}
			if ( cured )
			{
				playSoundEntity(this, 168, 128);
			}
		}
	}

	// unparalyze certain boss characters
	if ( myStats->EFFECTS[EFF_PARALYZED] && ((myStats->type >= LICH && myStats->type < KOBOLD)
		|| myStats->type == COCKATRICE || myStats->type == LICH_FIRE || myStats->type == LICH_ICE) )
	{
		myStats->EFFECTS[EFF_PARALYZED] = false;
		myStats->EFFECTS_TIMERS[EFF_PARALYZED] = 0;
	}

	// wake up
	if ( myStats->EFFECTS[EFF_ASLEEP] && (myStats->OLDHP != myStats->HP || (myStats->type >= LICH && myStats->type < KOBOLD)
		|| myStats->type == COCKATRICE || myStats->type == LICH_FIRE || myStats->type == LICH_ICE) )
	{
		messagePlayer(player, language[658]);
		if ( monsterAllyGetPlayerLeader() && monsterAllySpecial == ALLY_SPECIAL_CMD_REST )
		{
			// allies resting.
			if ( !naturalHeal )
			{
				myStats->EFFECTS[EFF_ASLEEP] = false; // wake up
				myStats->EFFECTS_TIMERS[EFF_ASLEEP] = 0;
				myStats->EFFECTS[EFF_HP_REGEN] = false; // stop regen
				myStats->EFFECTS_TIMERS[EFF_HP_REGEN] = 0;
				monsterAllySpecial = ALLY_SPECIAL_CMD_NONE;
			}
		}
		else
		{
			myStats->EFFECTS[EFF_ASLEEP] = false;
			myStats->EFFECTS_TIMERS[EFF_ASLEEP] = 0;
		}
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

	if ( (entitystats = this->getStats()) == nullptr )
	{
		return 0;
	}

	attack = BASE_MELEE_DAMAGE; // base attack strength
	if ( entitystats->weapon != nullptr )
	{
		attack += entitystats->weapon->weaponGetAttack();
	}
	else if ( entitystats->weapon == nullptr )
	{
		// bare handed.
		if ( entitystats->gloves )
		{
			if ( entitystats->gloves->type == BRASS_KNUCKLES )
			{
				attack += 1 + entitystats->gloves->beatitude;
			}
			else if ( entitystats->gloves->type == IRON_KNUCKLES )
			{
				attack += 2 + entitystats->gloves->beatitude;
			}
			else if ( entitystats->gloves->type == SPIKED_GAUNTLETS )
			{
				attack += 3 + entitystats->gloves->beatitude;
			}
		}
		if ( entitystats->ring )
		{
			attack += 1 + entitystats->ring->beatitude;
		}
	}
	attack += this->getSTR();

	return attack;
}

/*-------------------------------------------------------------------------------

Entity::getRangedAttack

returns the ranged attack power of an entity based on dex, ranged weapon, and a
base number

-------------------------------------------------------------------------------*/

Sint32 Entity::getRangedAttack()
{
	Stat* entitystats;
	int attack = BASE_RANGED_DAMAGE; // base ranged attack strength

	if ( (entitystats = this->getStats()) == nullptr )
	{
		return 0;
	}

	if ( entitystats->weapon )
	{
		attack += entitystats->weapon->weaponGetAttack();
		attack += getDEX();
		if ( behavior == &actMonster )
		{
			attack += getPER(); // monsters take PER into their ranged attacks to avoid having to increase their speed.
			attack += entitystats->PROFICIENCIES[PRO_RANGED] / 20; // 0 to 5 bonus attack for monsters
		}
	}
	else
	{
		return 0;
	}
	return attack;
}

/*-------------------------------------------------------------------------------

Entity::getThrownAttack

returns the thrown attack power of an entity based on dex, thrown weapon, and a
base number. For tooltip only.

-------------------------------------------------------------------------------*/

Sint32 Entity::getThrownAttack()
{
	Stat* entitystats;
	int attack = BASE_THROWN_DAMAGE; // base thrown attack strength

	if ( (entitystats = this->getStats()) == nullptr )
	{
		return attack;
	}

	if ( entitystats->weapon )
	{
		attack += entitystats->weapon->weaponGetAttack();
		attack += entitystats->PROFICIENCIES[PRO_RANGED] / 5; // 0 to 20 bonus attack.
	}
	else
	{
		return 0;
	}
	return attack;
}

/*-------------------------------------------------------------------------------

Entity::getBonusAttackOnTarget

returns the attack power depending on targets attributes, status effects and race

-------------------------------------------------------------------------------*/

Sint32 Entity::getBonusAttackOnTarget(Stat& hitstats)
{
	Stat* entitystats;
	Sint32 bonusAttack = 0;

	if ( (entitystats = this->getStats()) == nullptr )
	{
		return 0;
	}

	if ( entitystats->weapon )
	{
		if ( hitstats.EFFECTS_TIMERS[EFF_VAMPIRICAURA] )
		{
			// blessed weapons deal more damage under this effect.
			bonusAttack += entitystats->weapon->beatitude;
		}
	}

	return bonusAttack;
}

/*-------------------------------------------------------------------------------

Entity::getSTR()

returns the STR attribute of an entity, post modifiers

-------------------------------------------------------------------------------*/

Sint32 Entity::getSTR()
{
	Stat* entitystats;

	if ( (entitystats = this->getStats()) == nullptr )
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
	if ( entitystats->gloves != nullptr )
	{
		if ( entitystats->gloves->type == GAUNTLETS_STRENGTH )
		{
			if ( entitystats->gloves->beatitude >= 0 )
			{
				STR++;
			}
			STR += entitystats->gloves->beatitude;
		}
	}
	if ( entitystats->ring != nullptr )
	{
		if ( entitystats->ring->type == RING_STRENGTH )
		{
			if ( entitystats->ring->beatitude >= 0 )
			{
				STR++;
			}
			STR += entitystats->ring->beatitude;
		}
	}
	if ( entitystats->EFFECTS[EFF_DRUNK] )
	{
		switch ( entitystats->type )
		{
			case GOATMAN:
				STR += 10; //Goatman love booze.
				break;
			default:
				++STR;
				break;
		}
	}
	if ( entitystats->EFFECTS[EFF_SHRINE_RED_BUFF] )
	{
		STR += 8;
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

	if ( (entitystats = this->getStats()) == nullptr )
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
	if ( entitystats->EFFECTS[EFF_VAMPIRICAURA] && !entitystats->EFFECTS[EFF_FAST] && !entitystats->EFFECTS[EFF_SLOW] )
	{
		DEX += 5;
		if ( entitystats->type == VAMPIRE )
		{
			DEX += 3;
		}
	}
	else if ( entitystats->EFFECTS[EFF_FAST] && !entitystats->EFFECTS[EFF_SLOW] )
	{
		DEX += 10;
	}
	if ( entitystats->EFFECTS[EFF_STUNNED] )
	{
		//DEX -= 5;
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
	if ( entitystats->shoes != nullptr )
	{
		if ( entitystats->shoes->type == LEATHER_BOOTS_SPEED )
		{
			if ( entitystats->shoes->beatitude >= 0 )
			{
				DEX++;
			}
			DEX += entitystats->shoes->beatitude;
		}
	}
	if ( entitystats->gloves != nullptr )
	{
		if ( entitystats->gloves->type == GLOVES_DEXTERITY )
		{
			if ( entitystats->gloves->beatitude >= 0 )
			{
				DEX++;
			}
			DEX += entitystats->gloves->beatitude;
		}
	}
	if ( entitystats->EFFECTS[EFF_DRUNK] )
	{
		switch ( entitystats->type )
		{
			default:
				--DEX;
				break;
		}
	}
	if ( entitystats->EFFECTS[EFF_SHRINE_GREEN_BUFF] )
	{
		DEX += 8;
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

	if ( (entitystats = this->getStats()) == nullptr )
	{
		return 0;
	}
	return statGetCON(entitystats);
}

Sint32 statGetCON(Stat* entitystats)
{
	Sint32 CON;

	CON = entitystats->CON;
	if ( entitystats->ring != nullptr )
	{
		if ( entitystats->ring->type == RING_CONSTITUTION )
		{
			if ( entitystats->ring->beatitude >= 0 )
			{
				CON++;
			}
			CON += entitystats->ring->beatitude;
		}
	}
	if ( entitystats->gloves != nullptr )
	{
		if ( entitystats->gloves->type == BRACERS_CONSTITUTION )
		{
			if ( entitystats->gloves->beatitude >= 0 )
			{
				CON++;
			}
			CON += entitystats->gloves->beatitude;
		}
	}
	if ( entitystats->EFFECTS[EFF_SHRINE_RED_BUFF] )
	{
		CON += 8;
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

	if ( (entitystats = this->getStats()) == nullptr )
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
	if ( entitystats->helmet != nullptr )
	{
		if ( entitystats->helmet->type == HAT_WIZARD )
		{
			if ( entitystats->helmet->beatitude >= 0 )
			{
				INT++;
			}
			INT += entitystats->helmet->beatitude;
		}
		else if ( entitystats->helmet->type == ARTIFACT_HELM )
		{
			if ( entitystats->helmet->beatitude >= 0 )
			{
				INT += 8;
			}
			INT += entitystats->helmet->beatitude;
		}
	}
	if ( entitystats->EFFECTS[EFF_SHRINE_BLUE_BUFF] )
	{
		INT += 8;
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

	if ( (entitystats = this->getStats()) == nullptr )
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
	{
		if ( entitystats->mask->type == TOOL_GLASSES )
		{
			if ( entitystats->mask->beatitude >= 0 )
			{
				PER++;
			}
			PER += entitystats->mask->beatitude;
		}
		else if ( entitystats->mask->type == TOOL_BLINDFOLD
					|| entitystats->mask->type == TOOL_BLINDFOLD_TELEPATHY
					|| entitystats->mask->type == TOOL_BLINDFOLD_FOCUS )
		{
			PER -= 10;
			PER += entitystats->mask->beatitude;
		}
	}
	if ( entitystats->EFFECTS[EFF_SHRINE_GREEN_BUFF] )
	{
		PER += 8;
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

	if ( (entitystats = this->getStats()) == nullptr )
	{
		return 0;
	}
	return statGetCHR(entitystats);
}

Sint32 statGetCHR(Stat* entitystats)
{
	Sint32 CHR;

	CHR = entitystats->CHR;
	if ( entitystats->helmet != nullptr )
	{
		if ( entitystats->helmet->type == HAT_JESTER )
		{
			if ( entitystats->helmet->beatitude >= 0 )
			{
				CHR++;
			}
			CHR += entitystats->helmet->beatitude;
		}
	}
	if ( entitystats->ring != nullptr )
	{
		if ( entitystats->ring->type == RING_ADORNMENT )
		{
			if ( entitystats->ring->beatitude >= 0 )
			{
				CHR++;
			}
			CHR += entitystats->ring->beatitude;
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
	if ( (entitystats = this->getStats()) == nullptr )
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
	if ( entitystats->mask != nullptr )
		if ( entitystats->mask->type == TOOL_BLINDFOLD 
			|| entitystats->mask->type == TOOL_BLINDFOLD_TELEPATHY 
			|| entitystats->mask->type == TOOL_BLINDFOLD_FOCUS )
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

bool Entity::isInvisible() const
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
	{
		if ( entitystats->cloak->type == CLOAK_INVISIBILITY )
		{
			return true;
		}
	}

	// wearing invisibility ring
	if ( entitystats->ring != NULL )
	{
		if ( entitystats->ring->type == RING_INVISIBILITY )
		{
			return true;
		}
	}

	if ( this->behavior == &actPlayer )
	{
		if ( this->skill[2] >= 0 && this->skill[2] < MAXPLAYERS )
		{
			if ( skillCapstoneUnlockedEntity(PRO_STEALTH) && (stats[this->skill[2]]->sneaking && !stats[this->skill[2]]->defending) )
			{
				if ( this->skill[9] == 0 ) // player attack variable.
				{
					return true;
				}
			}
		}
	}
	else if ( skillCapstoneUnlockedEntity(PRO_STEALTH) )
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
	if ( (entitystats = getStats()) == nullptr )
	{
		return true;
	}

	if ( behavior == &actPlayer && entitystats->EFFECTS[EFF_PACIFY] )
	{
		return false;
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

	// stunned
	if ( entitystats->EFFECTS[EFF_STUNNED] )
	{
		return false;
	}

	if ( (entitystats->type == LICH_FIRE || entitystats->type == LICH_ICE)
		&& monsterLichBattleState < LICH_BATTLE_READY )
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
	if ( x < 0 || y < 0 || x > 255 || y > 255 )
	{
		return nullptr; // invalid grid reference!
	}
	return &TileEntityList.gridEntities[x][y];

//	list_t* return_val = NULL;
//
//	//Loop through the list.
//	//If the entity's x and y match the tile's x and y (correcting for the difference in the two x/y systems, of course), then the entity is on the tile.
//	//Traverse map.entities...
//	node_t* node = NULL;
//	node_t* node2 = NULL;
//#ifdef __ARM_NEON__
//	const int32x2_t xy = { x, y };
//#endif
//
//	for ( node = map.entities->first; node != NULL; node = node->next )
//	{
//		if ( node->element )
//		{
//			Entity* entity = (Entity*)node->element;
//			if ( entity ) {
//#ifdef __ARM_NEON__
//				uint32x2_t eqxy = vceq_s32(vcvt_s32_f32(vmul_n_f32(vld1_f32(&entity->x), 1.0f / 16.0f)), xy);
//				if ( eqxy[0] && eqxy[1] )
//#else
//				if ( (int)floor((entity->x / 16)) == x && (int)floor((entity->y / 16)) == y )   //Check if the current entity is on the tile.
//#endif
//				{
//					//Right. So. Create the list if it doesn't exist.
//					if ( !return_val )
//					{
//						return_val = (list_t*)malloc(sizeof(list_t));
//						return_val->first = NULL;
//						return_val->last = NULL;
//					}
//
//					//And add the current entity to it.
//					node2 = list_AddNodeLast(return_val);
//					node2->element = entity;
//					node2->deconstructor = &emptyDeconstructor;
//				}
//			}
//		}
//	}
//
//	return return_val;
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

	if ( !entities )
	{
		return;    //No use continuing of got no entities.
	}

	node_t* node = NULL;
	node_t* node2 = NULL;
	//Loop through the list of entities.
	for ( node = entities->first; node != NULL; node = node->next )
	{
		if ( node->element )
		{
			Entity* entity = (Entity*)node->element;
			//Check if the entity is an item.
			if ( entity && entity->behavior == &actItem )
			{
				//If this is the first item found, the list needs to be created.
				if ( !(*list) )
				{
					*list = (list_t*)malloc(sizeof(list_t));
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

	/*if ( entities )
	{
		list_FreeAll(entities);
		free(entities);
	}*/

	//return return_val;
}

/*-------------------------------------------------------------------------------

Entity::attack

Causes an entity to attack using whatever weapon it's holding

-------------------------------------------------------------------------------*/

void Entity::attack(int pose, int charge, Entity* target)
{
	Stat* hitstats = nullptr;
	Stat* myStats = nullptr;
	Entity* entity = nullptr;
	int player, playerhit = -1;
	double dist;
	int c, i;
	int weaponskill = -1;
	node_t* node = nullptr;
	double tangent;

	if ( (myStats = getStats()) == nullptr )
	{
		return;
	}

	// get the player number, if applicable
	if ( behavior == &actPlayer )
	{
		player = skill[2];
	}
	else
	{
		player = -1; // not a player
	}

	if ( multiplayer != CLIENT )
	{
		// animation
		if ( player >= 0 )
		{
			if ( stats[player]->weapon != nullptr )
			{
				players[player]->entity->skill[9] = pose; // PLAYER_ATTACK
			}
			else
			{
				players[player]->entity->skill[9] = 1; // special case for punch to eliminate spanking motion :p
			}
			players[player]->entity->skill[10] = 0; // PLAYER_ATTACKTIME
		}
		else
		{
			if ( pose >= MONSTER_POSE_MELEE_WINDUP1 && pose <= MONSTER_POSE_SPECIAL_WINDUP3 )
			{
				monsterAttack = pose;
				monsterAttackTime = 0;
				if ( multiplayer == SERVER )
				{
					// be sure to update the clients with the new wind-up pose.
					serverUpdateEntitySkill(this, 8);
					serverUpdateEntitySkill(this, 9);
				}
				return; // don't execute the attack, let the monster animation call the attack() function again.
			}
			else if ( (myStats->type == INCUBUS && pose == MONSTER_POSE_INCUBUS_TELEPORT)
				|| (myStats->type == VAMPIRE && (pose == MONSTER_POSE_VAMPIRE_DRAIN || pose == MONSTER_POSE_VAMPIRE_AURA_CHARGE))
				|| (myStats->type == LICH_FIRE && pose == MONSTER_POSE_MAGIC_CAST1)
				|| (myStats->type == LICH_ICE && pose == MONSTER_POSE_MAGIC_CAST1)
				|| (myStats->type == LICH_ICE 
						&& (monsterLichIceCastPrev == LICH_ATK_CHARGE_AOE 
							|| monsterLichIceCastPrev == LICH_ATK_RISING_RAIN
							|| monsterLichIceCastPrev == LICH_ATK_FALLING_DIAGONAL
							|| monsterState == MONSTER_STATE_LICH_CASTSPELLS
							)
					)
			)
			{
				// calls animation, but doesn't actually attack
				monsterAttack = pose;
				monsterAttackTime = 0;
				if ( multiplayer == SERVER )
				{
					// be sure to update the clients with the new wind-up pose.
					serverUpdateEntitySkill(this, 8);
					serverUpdateEntitySkill(this, 9);
				}
				return; // don't execute the attack, let the monster animation call the attack() function again.
			}
			else if ( myStats->type == VAMPIRE && pose == MONSTER_POSE_VAMPIRE_AURA_CAST )
			{
				monsterAttack = 0;
			}
			else if ( myStats->weapon != nullptr || myStats->type == CRYSTALGOLEM || myStats->type == COCKATRICE )
			{
				monsterAttack = pose;
			}
			else
			{
				monsterAttack = 1;    // punching
			}
			monsterAttackTime = 0;
		}

		if ( multiplayer == SERVER )
		{
			if ( player >= 0 && player < MAXPLAYERS )
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

		if ( myStats->type == SHADOW )
		{
			if ( myStats->EFFECTS[EFF_INVISIBLE] )
			{
				//Shadows lose invisibility when they attack.
				//TODO: How does this play with the passive invisibility?
				setEffect(EFF_INVISIBLE, false, 0, true);
			}
		}

		if ( behavior == &actMonster && monsterAllyIndex != -1 )
		{
			Entity* myTarget = uidToEntity(monsterTarget);
			if ( myTarget )
			{
				if ( myTarget->monsterAllyIndex != -1 || myTarget->behavior == &actPlayer )
				{
					this->monsterReleaseAttackTarget(true); // stop attacking player allies or players after this hit executes.
				}
			}
		}

		if ( myStats->weapon != nullptr )
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
						case MAGICSTAFF_SUMMON:
							castSpell(uid, &spell_summon, true, false);
							break;
						case MAGICSTAFF_STONEBLOOD:
							castSpell(uid, &spell_stoneblood, true, false);
							break;
						case MAGICSTAFF_BLEED:
							castSpell(uid, &spell_bleed, true, false);
							break;
						case MAGICSTAFF_CHARM:
							castSpell(uid, &spell_charmMonster, true, false);
							break;
						default:
							messagePlayer(player, "This is my wish stick! Wishy wishy wish!");
							break;
					}

					// magicstaffs deplete themselves for each use
					bool degradeWeapon = true;
					if ( myStats->type == SHADOW || myStats->type == LICH_FIRE || myStats->type == LICH_ICE )
					{
						degradeWeapon = false; //certain monster's weapons don't degrade.
					}
					bool forceDegrade = false;
					if ( degradeWeapon )
					{
						if ( myStats->weapon->type == MAGICSTAFF_CHARM )
						{
							if ( myStats->weapon->beatitude <= SERVICABLE )
							{
								forceDegrade = true;
							}
						}
					}

					if ( (rand() % 3 == 0 && degradeWeapon) || forceDegrade )
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
							if ( itemCategory(myStats->weapon) == MAGICSTAFF && myStats->weapon->beatitude < 0 )
							{
								steamAchievementClient(player, "BARONY_ACH_ONE_MANS_TRASH");
							}
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
						case SPELLBOOK_STONEBLOOD:
							castSpell(uid, &spell_stoneblood, true, false);
							break;
						case SPELLBOOK_BLEED:
							castSpell(uid, &spell_bleed, true, false);
							break;
						case SPELLBOOK_SUMMON:
							castSpell(uid, &spell_summon, true, false);
							break;
						case SPELLBOOK_ACID_SPRAY:
							castSpell(uid, &spell_acidSpray, true, false);
							break;
						case SPELLBOOK_STEAL_WEAPON:
							castSpell(uid, &spell_stealWeapon, true, false);
							break;
						case SPELLBOOK_DRAIN_SOUL:
							castSpell(uid, &spell_drainSoul, true, false);
							break;
						case SPELLBOOK_VAMPIRIC_AURA:
							castSpell(uid, &spell_vampiricAura, true, false);
							break;
						case SPELLBOOK_CHARM_MONSTER:
							castSpell(uid, &spell_charmMonster, true, false);
							break;
						//case SPELLBOOK_REFLECT_MAGIC: //TODO: Test monster support. Maybe better to just use a special ability that directly casts the spell.
						//castSpell(uid, &spell_reflectMagic, true, false)
						//break;
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
					entity = newEntity(78, 1, map.entities, nullptr); // rock
					playSoundEntity(this, 239 + rand() % 3, 96);
				}
				else if ( myStats->weapon->type == CROSSBOW )
				{
					entity = newEntity(167, 1, map.entities, nullptr); // bolt
					playSoundEntity(this, 239 + rand() % 3, 96);
				}
				else
				{
					entity = newEntity(166, 1, map.entities, nullptr); // arrow
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

				// set properties of the arrow.
				entity->setRangedProjectileAttack(*this, *myStats);
				return;
			}

			// potions & gems (throwing), and thrown weapons
			if ( itemCategory(myStats->weapon) == POTION || itemCategory(myStats->weapon) == GEM || itemCategory(myStats->weapon) == THROWN )
			{
				bool drankPotion = false;
				if ( myStats->type == GOATMAN && itemCategory(myStats->weapon) == POTION )
				{
					//Goatmen chug potions & then toss them at you.
					if ( myStats->weapon->type == POTION_BOOZE && !myStats->EFFECTS[EFF_DRUNK] )
					{
						item_PotionBooze(myStats->weapon, this, false);
						drankPotion = true;
					}
					else if ( myStats->weapon->type == POTION_HEALING )
					{
						item_PotionHealing(myStats->weapon, this, false);
						drankPotion = true;
					}
					else if ( myStats->weapon->type == POTION_EXTRAHEALING )
					{
						item_PotionExtraHealing(myStats->weapon, this, false);
						drankPotion = true;
					}
				}

				playSoundEntity(this, 75, 64);
				if ( drankPotion )
				{
					Item* emptyBottle = newItem(POTION_EMPTY, myStats->weapon->status, myStats->weapon->beatitude, 1, myStats->weapon->appearance, myStats->weapon->appearance, nullptr);
					entity = newEntity(itemModel(emptyBottle), 1, map.entities, nullptr); // thrown item
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
					entity->skill[10] = emptyBottle->type;
					entity->skill[11] = emptyBottle->status;
					entity->skill[12] = emptyBottle->beatitude;
					entity->skill[13] = 1;
					entity->skill[14] = emptyBottle->appearance;
					entity->skill[15] = emptyBottle->identified;
				}
				else
				{
					entity = newEntity(itemModel(myStats->weapon), 1, map.entities, nullptr); // thrown item
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
				}

				if ( itemCategory(myStats->weapon) == THROWN )
				{
					// thrown items have slightly faster velocities
					if ( (myStats->weapon->type == STEEL_CHAKRAM || myStats->weapon->type == CRYSTAL_SHURIKEN) )
					{
						if ( this->behavior == &actPlayer )
						{
							// todo: change velocity of chakram/shuriken?
							entity->vel_x = 6 * cos(players[player]->entity->yaw);
							entity->vel_y = 6 * sin(players[player]->entity->yaw);
							entity->vel_z = -.3;
						}
						else if ( this->behavior == &actMonster )
						{
							// todo: change velocity of chakram/shuriken?
							entity->vel_x = 6 * cos(this->yaw);
							entity->vel_y = 6 * sin(this->yaw);
							entity->vel_z = -.3;
						}
					}
					else
					{
						if ( this->behavior == &actPlayer )
						{
							entity->vel_x = 6 * cos(players[player]->entity->yaw);
							entity->vel_y = 6 * sin(players[player]->entity->yaw);
							entity->vel_z = -.3;
						}
						else if ( this->behavior == &actMonster )
						{
							entity->vel_x = 6 * cos(this->yaw);
							entity->vel_y = 6 * sin(this->yaw);
							entity->vel_z = -.3;
						}
					}
				}
				else
				{
					if ( this->behavior == &actPlayer )
					{
						entity->vel_x = 5 * cos(players[player]->entity->yaw);
						entity->vel_y = 5 * sin(players[player]->entity->yaw);
						entity->vel_z = -.5;
					}
					else if ( this->behavior == &actMonster )
					{
						entity->vel_x = 5 * cos(this->yaw);
						entity->vel_y = 5 * sin(this->yaw);
						entity->vel_z = -.5;
					}
				}

				//TODO: Refactor this so that we don't have to copy paste this check a million times whenever some-one uses up an item.
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
					myStats->weapon = nullptr;
				}
				return;
			}
		}

		// normal attacks
		if ( target == nullptr )
		{
			playSoundEntity(this, 23 + rand() % 5, 128); // whoosh noise
			dist = lineTrace(this, x, y, yaw, STRIKERANGE, 0, false);
		}
		else
		{
			hit.entity = target;
		}

		if ( hit.entity != nullptr )
		{
			if ( !(svFlags & SV_FLAG_FRIENDLYFIRE) )
			{
				// test for friendly fire
				if ( checkFriend(hit.entity) )
				{
					return;
				}
			}
			else if ( (myStats->type == LICH_FIRE && hit.entity->getRace() == LICH_ICE)
				|| (myStats->type == LICH_ICE && hit.entity->getRace() == LICH_FIRE) )
			{
				// friendship <3
				return;
			}

			bool previousMonsterState = -1;

			if ( hit.entity->behavior == &actBoulder )
			{
				if ( myStats->weapon != nullptr )
				{
					if ( myStats->weapon->type == TOOL_PICKAXE )
					{
						// spawn several rock items
						int i = 8 + rand() % 4;

						int c;
						for ( c = 0; c < i; c++ )
						{
							Entity* entity = newEntity(-1, 1, map.entities, nullptr); //Rock/item entity.
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

						if ( player >= 0 && (abs(hit.entity->vel_x) > 0.01 || abs(hit.entity->vel_y) > 0.01) )
						{
							// boulder rolling, check if rolling towards player.
							bool lastResort = false;
							int boulderDirection = 0;
							if ( abs(hit.entity->yaw - (PI / 2)) < 0.1 )
							{
								boulderDirection = 1;
							}
							else if ( abs(hit.entity->yaw - (PI)) < 0.1 )
							{
								boulderDirection = 2;
							}
							else if ( abs(hit.entity->yaw - (3 * PI / 2)) < 0.1 )
							{
								boulderDirection = 3;
							}

							switch ( boulderDirection )
							{
								case 0: // east
									if ( static_cast<int>(oy / 16) == static_cast<int>(y / 16)
										&& static_cast<int>(ox / 16) <= static_cast<int>(x / 16) )
									{
										lastResort = true;
									}
									break;
								case 1: // south
									if ( static_cast<int>(ox / 16) == static_cast<int>(x / 16)
										&& static_cast<int>(oy / 16) <= static_cast<int>(y / 16) )
									{
										lastResort = true;
									}
									break;
								case 2: // west
									if ( static_cast<int>(oy / 16) == static_cast<int>(y / 16)
										&& static_cast<int>(ox / 16) >= static_cast<int>(x / 16) )
									{
										lastResort = true;
									}
									break;
								case 3: // north
									if ( static_cast<int>(ox / 16) == static_cast<int>(x / 16)
										&& static_cast<int>(oy / 16) >= static_cast<int>(y / 16) )
									{
										lastResort = true;
									}
									break;
								default:
									break;
							}
							if ( lastResort )
							{
								steamAchievementClient(player, "BARONY_ACH_LAST_RESORT");
							}
						}

						// destroy the boulder
						playSoundEntity(hit.entity, 67, 128);
						list_RemoveNode(hit.entity->mynode);
						messagePlayer(player, language[663]);
						if ( rand() % 2 )
						{
							myStats->weapon->status = static_cast<Status>(myStats->weapon->status - 1);
							if ( myStats->weapon->status < BROKEN )
							{
								myStats->weapon->status = BROKEN; // bounds checking.
							}
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
							Entity* monster = nullptr;
							if ( rand() % 2 == 0 )
							{
								monster = summonMonster(INSECTOID, ox, oy);
							}
							else
							{
								monster = summonMonster(SCORPION, ox, oy);
							}
							if ( monster )
							{
								int c;
								for ( c = 0; c < MAXPLAYERS; c++ )
								{
									Uint32 color = SDL_MapRGB(mainsurface->format, 255, 128, 0);
									messagePlayerColor(c, color, language[406]);
								}
							}
							boulderSokobanOnDestroy(false);
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
				previousMonsterState = hit.entity->monsterState;
				if ( hit.entity->children.first != nullptr )
				{
					if ( hit.entity->children.first->next != nullptr )
					{
						hitstats = (Stat*)hit.entity->children.first->next->element;

						bool alertTarget = true;
						if ( behavior == &actMonster && monsterAllyIndex != -1 && hit.entity->monsterAllyIndex != -1 )
						{
							// if we're both allies of players, don't alert the hit target.
							alertTarget = false;
						}

						// alert the monster!
						if ( hit.entity->monsterState != MONSTER_STATE_ATTACK && (hitstats->type < LICH || hitstats->type >= SHOPKEEPER) )
						{
							if ( alertTarget )
							{
								hit.entity->monsterAcquireAttackTarget(*this, MONSTER_STATE_PATH, true);
							}
						}

						// alert other monsters too
						Entity* ohitentity = hit.entity;
						for ( node = map.creatures->first; node != nullptr && alertTarget; node = node->next ) //Only searching for monsters, so don't iterate full map.entities.
						{
							entity = (Entity*)node->element;
							if ( entity && entity->behavior == &actMonster && entity != ohitentity )
							{
								Stat* buddystats = entity->getStats();
								if ( buddystats != nullptr )
								{
									if ( entity->checkFriend(hit.entity) )
									{
										if ( entity->monsterState == MONSTER_STATE_WAIT )
										{
											tangent = atan2(entity->y - ohitentity->y, entity->x - ohitentity->x);
											lineTrace(ohitentity, ohitentity->x, ohitentity->y, tangent, 1024, 0, false);
											if ( hit.entity == entity )
											{
												Entity* attackTarget = uidToEntity(uid);
												if ( attackTarget )
												{
													entity->monsterAcquireAttackTarget(*attackTarget, MONSTER_STATE_PATH);
												}
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

				bool alertAllies = true;
				if ( behavior == &actMonster && monsterAllyIndex != -1 )
				{
					// if I'm a player ally, don't alert other allies.
					alertAllies = false;
				}

				// alert the player's followers!
				//Maybe should send a signal to each follower, with some kind of attached priority, which determines if they change their target to bumrush the player's assailant.
				for ( node = hitstats->FOLLOWERS.first; node != nullptr && alertAllies; node = node->next )
				{
					Uint32* c = (Uint32*)node->element;
					entity = uidToEntity(*c);
					Entity* ohitentity = hit.entity;
					if ( entity )
					{
						Stat* buddystats = entity->getStats();
						if ( buddystats != nullptr )
						{
							if ( entity->monsterState == MONSTER_STATE_WAIT || (entity->monsterState == MONSTER_STATE_HUNT && entity->monsterTarget != uid) ) // monster is waiting or hunting
							{
								Entity* attackTarget = uidToEntity(uid);
								if ( attackTarget )
								{
									entity->monsterAcquireAttackTarget(*attackTarget, MONSTER_STATE_PATH);
								}
							}
						}
					}
					hit.entity = ohitentity;
				}
			}
			else if ( hit.entity->behavior == &actDoor || hit.entity->behavior == &::actFurniture || hit.entity->behavior == &::actChest )
			{
				int axe = 0;
				if ( myStats->weapon )
				{
					if ( myStats->weapon->type == BRONZE_AXE || myStats->weapon->type == IRON_AXE || myStats->weapon->type == STEEL_AXE )
					{
						axe = 1; // axes do extra damage to doors :)
					}
				}
				if ( hit.entity->behavior != &::actChest )
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
				if ( (hit.entity->behavior != &::actChest && hit.entity->skill[4] > 0) || (hit.entity->behavior == &::actChest && hit.entity->skill[3] > 0) )
				{
					if ( hit.entity->behavior == &actDoor )
					{
						messagePlayer(player, language[666]);
					}
					else if ( hit.entity->behavior == &::actChest )
					{
						messagePlayer(player, language[667]);
					}
					else if ( hit.entity->behavior == &::actFurniture )
					{
						switch ( hit.entity->furnitureType )
						{
							case FURNITURE_CHAIR:
								messagePlayer(player, language[669]);
								break;
							case FURNITURE_TABLE:
								messagePlayer(player, language[668]);
								break;
							case FURNITURE_BED:
								messagePlayer(player, language[2509], language[2505]);
								break;
							case FURNITURE_BUNKBED:
								messagePlayer(player, language[2509], language[2506]);
								break;
							case FURNITURE_PODIUM:
								messagePlayer(player, language[2509], language[2507]);
								break;
							default:
								break;
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
					else if ( hit.entity->behavior == &::actChest )
					{
						messagePlayer(player, language[671]);
					}
					else if ( hit.entity->behavior == &::actFurniture )
					{
						switch ( hit.entity->furnitureType )
						{
							case FURNITURE_CHAIR:
								messagePlayer(player, language[673]);
								break;
							case FURNITURE_TABLE:
								messagePlayer(player, language[672]);
								break;
							case FURNITURE_BED:
								messagePlayer(player, language[2510], language[2505]);
								break;
							case FURNITURE_BUNKBED:
								messagePlayer(player, language[2510], language[2506]);
								break;
							case FURNITURE_PODIUM:
								messagePlayer(player, language[2510], language[2507]);
								break;
							default:
								break;
						}
					}
				}
				if ( hit.entity->behavior == &actDoor )
				{
					updateEnemyBar(this, hit.entity, language[674], hit.entity->skill[4], hit.entity->skill[9]);
				}
				else if ( hit.entity->behavior == &::actChest )
				{
					updateEnemyBar(this, hit.entity, language[675], hit.entity->skill[3], hit.entity->skill[8]);
				}
				else if ( hit.entity->behavior == &::actFurniture )
				{
					switch ( hit.entity->furnitureType )
					{
						case FURNITURE_CHAIR:
							updateEnemyBar(this, hit.entity, language[677], hit.entity->furnitureHealth, hit.entity->furnitureMaxHealth);
							break;
						case FURNITURE_TABLE:
							updateEnemyBar(this, hit.entity, language[676], hit.entity->furnitureHealth, hit.entity->furnitureMaxHealth);
							break;
						case FURNITURE_BED:
							updateEnemyBar(this, hit.entity, language[2505], hit.entity->furnitureHealth, hit.entity->furnitureMaxHealth);
							break;
						case FURNITURE_BUNKBED:
							updateEnemyBar(this, hit.entity, language[2506], hit.entity->furnitureHealth, hit.entity->furnitureMaxHealth);
							break;
						case FURNITURE_PODIUM:
							updateEnemyBar(this, hit.entity, language[2507], hit.entity->furnitureHealth, hit.entity->furnitureMaxHealth);
							break;
						default:
							break;
					}
				}
			}
			else if ( hit.entity->behavior == &actSink )
			{
				playSoundEntity(hit.entity, 28, 64);
				playSoundEntity(hit.entity, 140 + rand(), 64);
				messagePlayer(player, language[678]);
				if ( hit.entity->skill[0] > 0 )
				{
					hit.entity->skill[0]--; //Deplete one usage.

											//50% chance spawn a slime.
					if ( rand() % 2 == 0 )
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

					if ( hit.entity->skill[0] == 0 )   //Depleted.
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

			if ( hitstats != nullptr )
			{
				// hit chance
				//int hitskill=5; // for unarmed combat

				weaponskill = getWeaponSkill(myStats->weapon);

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
					if ( weaponskill >= PRO_SWORD && weaponskill <= PRO_POLEARM )
						if ( rand() % 10 == 0 )
						{
							this->increaseSkill(weaponskill);
						}

					// calculate and perform damage to opponent
					int damage = 0;
					int damagePreMultiplier = 1;

					if ( (myStats->type == CRYSTALGOLEM && pose == MONSTER_POSE_GOLEM_SMASH )
						|| (myStats->type == LICH_FIRE && pose == 3) )
					{
						damagePreMultiplier = 2;
					}

					if ( weaponskill >= 0 )
					{
						damage = std::max(0, (getAttack() * damagePreMultiplier) + getBonusAttackOnTarget(*hitstats) - AC(hitstats)) * damagetables[hitstats->type][weaponskill - PRO_SWORD];
					}
					else
					{
						damage = std::max(0, (getAttack() * damagePreMultiplier) + getBonusAttackOnTarget(*hitstats) - AC(hitstats));
					}
					if ( weaponskill == PRO_AXE )
					{
						damage++;
					}
					if ( myStats->type == LICH_FIRE && !hitstats->defending )
					{
						if ( damage <= 8 )
						{
							damage += (8 - damage) + rand() % 9; // 8 - 16 minimum damage.
						}
					}
					if ( behavior == &actMonster && myStats->EFFECTS[EFF_VAMPIRICAURA] )
					{
						damage += 5; // 5 bonus damage after reductions.
					}

					bool backstab = false;
					bool flanking = false;
					if ( player >= 0 )
					{
						real_t hitAngle = hit.entity->yawDifferenceFromPlayer(player);
						if ( (hitAngle >= 0 && hitAngle <= 2 * PI / 3) ) // 120 degree arc
						{
							int stealthCapstoneBonus = 1; 
							if ( skillCapstoneUnlockedEntity(PRO_STEALTH) )
							{
								stealthCapstoneBonus = 2;
							}
							
							if ( previousMonsterState == MONSTER_STATE_WAIT
								|| previousMonsterState == MONSTER_STATE_PATH )
							{
								// unaware monster, get backstab damage.
								backstab = true;
								damage += (stats[player]->PROFICIENCIES[PRO_STEALTH] / 20 + 2) * (2 * stealthCapstoneBonus);
								if ( rand() % 4 > 0 )
								{
									this->increaseSkill(PRO_STEALTH);
								}
							}
							else if ( rand() % 2 == 0 )
							{
								// monster currently engaged in some form of combat maneuver
								// 1 in 2 chance to flank defenses.
								flanking = true;
								damage += (stats[player]->PROFICIENCIES[PRO_STEALTH] / 20 + 1) * (stealthCapstoneBonus);
								if ( rand() % 20 == 0 )
								{
									this->increaseSkill(PRO_STEALTH);
								}
							}
						}
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
					{
						if ( myStats->weapon->type == ARTIFACT_AXE )
						{
							if ( rand() % 3 == 0 )
							{
								damage *= 2;    // Parashu sometimes doubles damage
							}
						}
					}

					hit.entity->modHP(-damage); // do the damage

					// write the obituary
					killedByMonsterObituary(hit.entity);

					// update enemy bar for attacker
					if ( !strcmp(hitstats->name, "") )
					{
						if ( hitstats->type < KOBOLD ) //Original monster count
						{
							updateEnemyBar(this, hit.entity, language[90 + hitstats->type], hitstats->HP, hitstats->MAXHP);
						}
						else if ( hitstats->type >= KOBOLD ) //New monsters
						{
							updateEnemyBar(this, hit.entity, language[2000 + (hitstats->type - KOBOLD)], hitstats->HP, hitstats->MAXHP);
						}
					}
					else
					{
						updateEnemyBar(this, hit.entity, hitstats->name, hitstats->HP, hitstats->MAXHP);
					}

					// damage weapon if applicable

					bool isWeakWeapon = false;
					bool artifactWeapon = false;
					bool degradeWeapon = false;
					ItemType weaponType = static_cast<ItemType>(WOODEN_SHIELD);

					if ( myStats->weapon != NULL )
					{
						weaponType = myStats->weapon->type;
						if ( weaponType == ARTIFACT_AXE || weaponType == ARTIFACT_MACE || weaponType == ARTIFACT_SPEAR || weaponType == ARTIFACT_SWORD )
						{
							artifactWeapon = true;
						}
						else if ( weaponType == CRYSTAL_BATTLEAXE || weaponType == CRYSTAL_MACE || weaponType == CRYSTAL_SWORD || weaponType == CRYSTAL_SPEAR )
						{
							// crystal weapons degrade faster.
							isWeakWeapon = true;
						}

						if ( !artifactWeapon )
						{
							// crystal weapons chance to not degrade 66% chance on 0 dmg, else 96%
							if ( isWeakWeapon && ((rand() % 3 == 0 && damage == 0) || (rand() % 25 == 0 && damage > 0)) )
							{
								degradeWeapon = true;
							}
							// other weapons chance to not degrade 75% chance on 0 dmg, else 98%
							else if ( !isWeakWeapon && ((rand() % 4 == 0 && damage == 0) || (rand() % 50 == 0 && damage > 0)) )
							{
								degradeWeapon = true;
							}

							if ( myStats->type == SHADOW || myStats->type == LICH_FIRE || myStats->type == LICH_ICE )
							{
								degradeWeapon = false; //certain monster's weapons don't degrade.
							}

							if ( myStats->type == SKELETON && monsterAllySummonRank != 0 )
							{
								degradeWeapon = false;
							}

							if ( degradeWeapon )
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
								if ( myStats->weapon->status == BROKEN && behavior == &actMonster && playerhit >= 0 )
								{
									if ( playerhit > 0 )
									{
										steamStatisticUpdateClient(playerhit, STEAM_STAT_TOUGH_AS_NAILS, STEAM_STAT_INT, 1);
									}
									else
									{
										steamStatisticUpdate(STEAM_STAT_TOUGH_AS_NAILS, STEAM_STAT_INT, 1);
									}
								}
							}
						}
					}

					// damage opponent armor if applicable
					Item* armor = NULL;
					int armornum = 0;
					bool isWeakArmor = false;

					if ( damage > 0 )
					{
						// choose random piece of equipment to target
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

						if ( armor != NULL && armor->status > BROKEN )
						{
							switch ( armor->type )
							{
								case CRYSTAL_HELM:
								case CRYSTAL_SHIELD:
								case CRYSTAL_BREASTPIECE:
								case CRYSTAL_BOOTS:
								case CRYSTAL_GLOVES:
									isWeakArmor = true;
									break;
								default:
									isWeakArmor = false;
									break;
							}
						}

						if ( weaponskill == PRO_MACE )
						{
							if ( isWeakArmor )
							{
								// 80% chance to be deselected from degrading.
								if ( rand() % 5 > 0 )
								{
									armor = NULL;
									armornum = 0;
								}
							}
							else
							{
								// 90% chance to be deselected from degrading.
								if ( rand() % 10 > 0 )
								{
									armor = NULL;
									armornum = 0;
								}
							}
						}
						// crystal golem special attack increase chance for armor to break if hit. (25-33%)
						// special attack only degrades armor if primary target.
						else if ( pose == MONSTER_POSE_GOLEM_SMASH && target == nullptr )
						{
							if ( isWeakArmor )
							{
								// 66% chance to be deselected from degrading.
								if ( rand() % 3 > 0 )
								{
									armor = NULL;
									armornum = 0;
								}
							}
							else
							{
								// 75% chance to be deselected from degrading.
								if ( rand() % 4 > 0 )
								{
									armor = NULL;
									armornum = 0;
								}
							}
						}
						else
						{
							if ( isWeakArmor )
							{
								// 93% chance to be deselected from degrading.
								if ( rand() % 15 > 0 )
								{
									armor = NULL;
									armornum = 0;
								}
							}
							else
							{
								// 96% chance to be deselected from degrading.
								if ( rand() % 25 > 0 )
								{
									armor = NULL;
									armornum = 0;
								}
							}
						}
					}

					// if nothing chosen to degrade, check extra shield chances to degrade
					if ( hitstats->shield != NULL && hitstats->shield->status > BROKEN && armor == NULL )
					{
						if ( hitstats->shield->type == TOOL_CRYSTALSHARD && hitstats->defending )
						{
							// shards degrade by 1 stage each hit.
							armor = hitstats->shield;
							armornum = 4;
						}
						else if ( hitstats->shield->type == MIRROR_SHIELD && hitstats->defending )
						{
							// mirror shield degrade by 1 stage each hit.
							armor = hitstats->shield;
							armornum = 4;
						}
						else
						{
							// if no armor piece was chosen to break, grant chance to improve shield skill.
							if ( itemCategory(hitstats->shield) == ARMOR )
							{
								if ( (rand() % 15 == 0 && damage > 0) || (damage == 0 && rand() % 8 == 0) )
								{
									hit.entity->increaseSkill(PRO_SHIELD); // increase shield skill
								}
							}

							// shield still has chance to degrade after raising skill.
							// crystal golem special attack increase chance for shield to break if defended. (33%)
							// special attack only degrades shields if primary target.
							if ( (hitstats->defending && rand() % 10 == 0)
								|| (hitstats->defending && pose == MONSTER_POSE_GOLEM_SMASH && target == nullptr && rand() % 3 == 0)
								&& armor == NULL )
							{
								armor = hitstats->shield;
								armornum = 4;
							}
						}
					}

					if ( armor != NULL && armor->status > BROKEN )
					{
						hit.entity->degradeArmor(*hitstats, *armor, armornum);
						if ( armor->status == BROKEN )
						{
							if ( player >= 0 && hit.entity->behavior == &actMonster )
							{
								if ( player > 0 )
								{
									steamStatisticUpdateClient(player, STEAM_STAT_UNSTOPPABLE_FORCE, STEAM_STAT_INT, 1);
								}
								else
								{
									steamStatisticUpdate(STEAM_STAT_UNSTOPPABLE_FORCE, STEAM_STAT_INT, 1);
								}
							}
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

								// Attempt to set the Entity on fire
								hit.entity->SetEntityOnFire();

								// If a Player was hit, and they are now on fire, tell them what set them on fire
								if ( playerhit > 0 && hit.entity->flags[BURNING] )
								{
									messagePlayer(playerhit, language[683]); // "Dyrnwyn sets you on fire!"
								}
							}
						}
					}

					bool statusInflicted = false;

					// special monster effects
					if ( myStats->type == CRYSTALGOLEM && pose == MONSTER_POSE_GOLEM_SMASH )
					{
						if ( damage >= 150 && playerhit >= 0 )
						{
							if ( hitstats && hitstats->HP > 0 )
							{
								steamAchievementClient(playerhit, "BARONY_ACH_SPONGE");
							}
						}
						if ( multiplayer != CLIENT )
						{
							createParticleRock(hit.entity);
							if ( multiplayer == SERVER )
							{
								serverSpawnMiscParticles(hit.entity, PARTICLE_EFFECT_ABILITY_ROCK, 0);
							}
							if ( target == nullptr )
							{
								// only play sound once on primary target.
								playSoundEntity(hit.entity, 181, 64);
							}
						}
					}
					else if ( (damage > 0 || hitstats->EFFECTS[EFF_PACIFY]) && rand() % 4 == 0 )
					{
						int armornum = 0;
						Item* armor = NULL;
						int armorstolen = rand() % 9;
						switch ( myStats->type )
						{
							case SCORPION:
								hitstats->EFFECTS[EFF_PARALYZED] = true;
								hitstats->EFFECTS_TIMERS[EFF_PARALYZED] = std::max(50, 150 - hit.entity->getCON() * 5);
								messagePlayer(playerhit, language[684]);
								messagePlayer(playerhit, language[685]);
								serverUpdateEffects(playerhit);
								break;
							case SPIDER:
								hitstats->EFFECTS[EFF_POISONED] = true;
								hitstats->EFFECTS_TIMERS[EFF_POISONED] = std::max(200, 600 - hit.entity->getCON() * 20);
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
									Item* stolenArmor = newItem(armor->type, armor->status, armor->beatitude, armor->count, armor->appearance, armor->identified, &myStats->inventory);
									stolenArmor->ownerUid = hit.entity->getUID();
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
									monsterState = 0;
									monsterTarget = 0;
								}
								break;
							default:
								break;
						}
					}
					else if ( damage == 0 && !(hitstats->defending) )
					{
						// special chance effects when damage is 0.
						if ( rand() % 20 == 0 )
						{
							switch ( myStats->type )
							{
								case SCORPION:
									hitstats->EFFECTS[EFF_PARALYZED] = true;
									hitstats->EFFECTS_TIMERS[EFF_PARALYZED] = std::max(50, 150 - hit.entity->getCON() * 5);
									messagePlayer(playerhit, language[684]);
									messagePlayer(playerhit, language[685]);
									serverUpdateEffects(playerhit);
									statusInflicted = true;
									break;
								case SPIDER:
									hitstats->EFFECTS[EFF_POISONED] = true;
									hitstats->EFFECTS_TIMERS[EFF_POISONED] = std::max(200, 300 - hit.entity->getCON() * 20);
									messagePlayer(playerhit, language[686]);
									messagePlayer(playerhit, language[687]);
									serverUpdateEffects(playerhit);
									statusInflicted = true;
									break;
								default:
									break;
							}
						}
					}

					if ( player >= 0 && hit.entity->behavior == &actMonster )
					{
						if ( damage > 0 )
						{
							updateAchievementRhythmOfTheKnight(player, hit.entity, false);
						}
						else
						{
							if ( !achievementStatusRhythmOfTheKnight[player] )
							{
								achievementRhythmOfTheKnightVec[player].clear(); // didn't inflict damage.
							}
						}
					}

					// send messages
					if ( !strcmp(hitstats->name, "") )
					{
						Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
						if ( hitstats->HP > 0 )
						{
							if ( damage > olddamage )
							{
								// critical hit
								messagePlayerMonsterEvent(player, color, *hitstats, language[689], language[689], MSG_COMBAT);
							}
							else
							{
								// normal hit
								messagePlayerMonsterEvent(player, color, *hitstats, language[690], language[690], MSG_COMBAT);
							}
							if ( damage == 0 )
							{
								// blow bounces off
								messagePlayer(player, language[691]);
							}
							else
							{
								if ( flanking )
								{
									// flank defenses
									messagePlayerMonsterEvent(player, color, *hitstats, language[2545], language[2545], MSG_COMBAT);
								}
								else if ( backstab )
								{
									// backstab on unaware enemy
									messagePlayerMonsterEvent(player, color, *hitstats, language[2543], language[2543], MSG_COMBAT);
								}
							}
						}
						else
						{
							// HP <= 0
							if ( backstab )
							{
								// assassinate monster
								messagePlayerMonsterEvent(player, color, *hitstats, language[2547], language[2547], MSG_COMBAT);
								if ( hitstats->type == COCKATRICE )
								{
									steamAchievementClient(player, "BARONY_ACH_SCALES_IN_FAVOR");
								}
							}
							else
							{
								// kill monster
								messagePlayerMonsterEvent(player, color, *hitstats, language[692], language[692], MSG_COMBAT);
								if ( player >= 0 && hit.entity && hit.entity->behavior == &actMonster )
								{
									real_t hitAngle = hit.entity->yawDifferenceFromPlayer(player);
									if ( (hitAngle >= 0 && hitAngle <= 2 * PI / 3) ) // 120 degree arc
									{
										if ( hit.entity->monsterState == MONSTER_STATE_ATTACK && hit.entity->monsterTarget != 0
											&& hit.entity->monsterTarget != getUID() )
										{
											bool angelOfDeath = false;
											// monster is attacking another entity.
											for ( int i = 0; i < MAXPLAYERS; ++i )
											{
												if ( players[i] && players[i]->entity )
												{
													if ( players[i]->entity->getUID() == hit.entity->monsterTarget )
													{
														// monster is attacking another player.
														angelOfDeath = true;
														break;
													}
													Entity* tmpEnt = uidToEntity(hit.entity->monsterTarget);
													if ( tmpEnt )
													{
														Stat* tmpStats = tmpEnt->getStats();
														if ( tmpStats )
														{
															if ( tmpStats->leader_uid == players[i]->entity->getUID() )
															{
																// monster is attacking an allied NPC of a player.
																angelOfDeath = true;
																break;
															}
														}
													}
												}
											}
											if ( angelOfDeath )
											{
												steamAchievementClient(player, "BARONY_ACH_ANGEL_OF_DEATH");
											}
										}
									}
								}
							}
							awardXP(hit.entity, true, true);
						}
					}
					else
					{
						Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
						if ( hitstats->HP > 0 )
						{
							if ( damage > olddamage )
							{
								// critical hit
								messagePlayerMonsterEvent(player, color, *hitstats, language[689], language[693], MSG_COMBAT);
							}
							else
							{
								// normal hit
								messagePlayerMonsterEvent(player, color, *hitstats, language[690], language[694], MSG_COMBAT);
							}
							if ( damage == 0 )
							{
								// blow bounces off
								if ( hitstats->sex )
								{
									messagePlayerMonsterEvent(player, 0xFFFFFFFF, *hitstats, language[691], language[695], MSG_COMBAT);
								}
								else
								{
									messagePlayerMonsterEvent(player, 0xFFFFFFFF, *hitstats, language[691], language[696], MSG_COMBAT);
								}
							}
							else
							{
								if ( flanking )
								{
									// flank defenses
									messagePlayerMonsterEvent(player, color, *hitstats, language[2545], language[2546], MSG_COMBAT);
								}
								else if ( backstab )
								{
									// backstab on unaware enemy
									messagePlayerMonsterEvent(player, color, *hitstats, language[2543], language[2544], MSG_COMBAT);
								}
							}
						}
						else
						{
							// HP <= 0
							if ( backstab )
							{
								// assassinate monster
								messagePlayerMonsterEvent(player, color, *hitstats, language[2547], language[2548], MSG_COMBAT);
								if ( hitstats->type == COCKATRICE )
								{
									steamAchievementClient(player, "BARONY_ACH_SCALES_IN_FAVOR");
								}
							}
							else
							{
								// kill monster
								messagePlayerMonsterEvent(player, color, *hitstats, language[692], language[697], MSG_COMBAT);
								if ( player >= 0 && hit.entity && hit.entity->behavior == &actMonster )
								{
									real_t hitAngle = hit.entity->yawDifferenceFromPlayer(player);
									if ( (hitAngle >= 0 && hitAngle <= 2 * PI / 3) ) // 120 degree arc
									{
										if ( hit.entity->monsterState == MONSTER_STATE_ATTACK && hit.entity->monsterTarget != 0
											&& hit.entity->monsterTarget != getUID() )
										{
											bool angelOfDeath = false;
											// monster is attacking another entity.
											for ( int i = 0; i < MAXPLAYERS; ++i )
											{
												if ( players[i] && players[i]->entity )
												{
													if ( players[i]->entity->getUID() == hit.entity->monsterTarget )
													{
														// monster is attacking another player.
														angelOfDeath = true;
														break;
													}
													Entity* tmpEnt = uidToEntity(hit.entity->monsterTarget);
													if ( tmpEnt )
													{
														Stat* tmpStats = tmpEnt->getStats();
														if ( tmpStats )
														{
															if ( tmpStats->leader_uid == players[i]->entity->getUID() )
															{
																// monster is attacking an allied NPC of a player.
																angelOfDeath = true;
																break;
															}
														}
													}
												}
											}
											if ( angelOfDeath )
											{
												steamAchievementClient(player, "BARONY_ACH_ANGEL_OF_DEATH");
											}
										}
									}
								}
							}
							awardXP(hit.entity, true, true);
						}
					}
					if ( playerhit > 0 && multiplayer == SERVER )
					{
						if ( pose == MONSTER_POSE_GOLEM_SMASH )
						{
							if ( target == nullptr )
							{
								// primary target
								strcpy((char*)net_packet->data, "SHAK");
								net_packet->data[4] = 20; // turns into .2
								net_packet->data[5] = 20;
								net_packet->address.host = net_clients[playerhit - 1].host;
								net_packet->address.port = net_clients[playerhit - 1].port;
								net_packet->len = 6;
								sendPacketSafe(net_sock, -1, net_packet, playerhit - 1);
							}
							else
							{
								// secondary target
								strcpy((char*)net_packet->data, "SHAK");
								net_packet->data[4] = 10; // turns into .1
								net_packet->data[5] = 10;
								net_packet->address.host = net_clients[playerhit - 1].host;
								net_packet->address.port = net_clients[playerhit - 1].port;
								net_packet->len = 6;
								sendPacketSafe(net_sock, -1, net_packet, playerhit - 1);
							}

							strcpy((char*)net_packet->data, "UPHP");
							SDLNet_Write32((Uint32)hitstats->HP, &net_packet->data[4]);
							SDLNet_Write32((Uint32)myStats->type, &net_packet->data[8]);
							net_packet->address.host = net_clients[playerhit - 1].host;
							net_packet->address.port = net_clients[playerhit - 1].port;
							net_packet->len = 12;
							sendPacketSafe(net_sock, -1, net_packet, playerhit - 1);
						}
						else
						{
							strcpy((char*)net_packet->data, "UPHP");
							SDLNet_Write32((Uint32)hitstats->HP, &net_packet->data[4]);
							SDLNet_Write32((Uint32)myStats->type, &net_packet->data[8]);
							net_packet->address.host = net_clients[playerhit - 1].host;
							net_packet->address.port = net_clients[playerhit - 1].port;
							net_packet->len = 12;
							sendPacketSafe(net_sock, -1, net_packet, playerhit - 1);
						}
					}
					else if ( playerhit == 0 )
					{
						if ( pose == MONSTER_POSE_GOLEM_SMASH )
						{
							if ( target == nullptr )
							{
								// primary target
								camera_shakex += .2;
								camera_shakey += 20;
							}
							else
							{
								// secondary target
								camera_shakex += .1;
								camera_shakey += 10;
							}
						}
						else if ( damage > 0 )
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

					if ( damage > 0 )
					{
						Entity* gib = spawnGib(hit.entity);
						serverSpawnGibForClient(gib);
						Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
						messagePlayerMonsterEvent(playerhit, color, *myStats, language[698], language[699], MSG_ATTACKS);
						if ( playerhit >= 0 )
						{
							if ( !achievementStatusRhythmOfTheKnight[playerhit] )
							{
								achievementRhythmOfTheKnightVec[playerhit].clear();
							}
							if ( !achievementStatusThankTheTank[playerhit] )
							{
								achievementThankTheTankPair[playerhit] = std::make_pair(0, 0);
							}
							//messagePlayer(0, "took damage!");
						}
					}
					else
					{
						// display 'blow bounces off' message
						//messagePlayer(playerhit, language[700]);
						if ( !statusInflicted )
						{
							messagePlayerMonsterEvent(playerhit, 0xFFFFFFFF, *myStats, language[2457], language[2458], MSG_COMBAT);
						}
						if ( myStats->type == COCKATRICE && hitstats->defending )
						{
							steamAchievementClient(playerhit, "BARONY_ACH_COCK_BLOCK");
						}
						else if ( myStats->type == MINOTAUR && !hitstats->defending )
						{
							steamAchievementClient(playerhit, "BARONY_ACH_ONE_WHO_KNOCKS");
						}
						if ( playerhit >= 0 )
						{
							if ( hitstats->defending )
							{
								updateAchievementRhythmOfTheKnight(playerhit, this, true);
								updateAchievementThankTheTank(playerhit, this, false);
							}
							else if ( !achievementStatusRhythmOfTheKnight[player] )
							{
								achievementRhythmOfTheKnightVec[playerhit].clear();
								//messagePlayer(0, "used AC!");
							}
						}
					}

					playSoundEntity(hit.entity, 28, 64);

					// chance of bleeding
					bool wasBleeding = hitstats->EFFECTS[EFF_BLEEDING]; // check if currently bleeding when this roll occurred.
					if ( gibtype[(int)hitstats->type] == 1 )
					{
						if ( hitstats->HP > 5 && damage > 0 )
						{
							if ( (rand() % 20 == 0 && weaponskill != PRO_SWORD)
								|| (rand() % 10 == 0 && weaponskill == PRO_SWORD)
								|| (rand() % 4 == 0 && pose == MONSTER_POSE_GOLEM_SMASH)
								|| (rand() % 10 == 0 && myStats->type == VAMPIRE && myStats->weapon == nullptr)
								|| (rand() % 8 == 0 && myStats->EFFECTS_TIMERS[EFF_VAMPIRICAURA] && (myStats->weapon == nullptr || myStats->type == LICH_FIRE))
							)
							{
								bool heavyBleedEffect = false; // heavy bleed will have a greater starting duration, and add to existing duration.
								if ( pose == MONSTER_POSE_GOLEM_SMASH )
								{
									heavyBleedEffect = true;
								}
								else if ( myStats->type == VAMPIRE || myStats->EFFECTS_TIMERS[EFF_VAMPIRICAURA] )
								{
									if ( rand() % 2 == 0 ) // 50% for heavy bleed effect.
									{
										heavyBleedEffect = true;
									}
								}

								char playerHitMessage[1024] = "";
								char monsterHitMessage[1024] = "";

								if ( !wasBleeding && !heavyBleedEffect )
								{
									// normal bleed effect
									hitstats->EFFECTS_TIMERS[EFF_BLEEDING] = std::max(480 + rand() % 360 - hit.entity->getCON() * 100, 120); // 2.4-16.8 seconds
									hitstats->EFFECTS[EFF_BLEEDING] = true;
									strcpy(playerHitMessage, language[701]);
									if ( !strcmp(hitstats->name, "") )
									{
										strcpy(monsterHitMessage, language[702]);
									}
									else
									{
										strcpy(monsterHitMessage, language[703]);
									}
								}
								else if ( heavyBleedEffect )
								{
									if ( !wasBleeding )
									{
										hitstats->EFFECTS_TIMERS[EFF_BLEEDING] = std::max(500 + rand() % 500 - hit.entity->getCON() * 10, 250); // 5-20 seconds
										hitstats->EFFECTS[EFF_BLEEDING] = true;
										strcpy(playerHitMessage, language[2451]);
										if ( !strcmp(hitstats->name, "") )
										{
											strcpy(monsterHitMessage, language[2452]);
										}
										else
										{
											strcpy(monsterHitMessage, language[2453]);
										}
									}
									else
									{
										hitstats->EFFECTS_TIMERS[EFF_BLEEDING] += std::max(rand() % 350 - hit.entity->getCON() * 5, 100); // 2-7 seconds in addition
										hitstats->EFFECTS[EFF_BLEEDING] = true;
										strcpy(playerHitMessage, language[2454]);
										if ( !strcmp(hitstats->name, "") )
										{
											strcpy(monsterHitMessage, language[2455]);
										}
										else
										{
											strcpy(monsterHitMessage, language[2456]);
										}
									}
								}

								// message player of effect, skip if hit entity was already bleeding.
								if ( hitstats->EFFECTS[EFF_BLEEDING] && (!wasBleeding || heavyBleedEffect) )
								{
									if ( heavyBleedEffect )
									{
										hitstats->EFFECTS[EFF_SLOW] = true;
										hitstats->EFFECTS_TIMERS[EFF_SLOW] = 60;
									}
									if ( hit.entity->behavior == &actPlayer && multiplayer == SERVER )
									{
										serverUpdateEffects(hit.entity->skill[2]);
									}
									if ( playerhit >= 0 )
									{
										Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
										messagePlayerColor(playerhit, color, playerHitMessage);
									}
									else
									{
										Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
										if ( !strcmp(hitstats->name, "") )
										{
											messagePlayerColor(player, color, monsterHitMessage, hit.entity->getMonsterLangEntry());
										}
										else
										{
											messagePlayerColor(player, color, monsterHitMessage, hitstats->name);
										}
									}
								}
							}
						}
					}
					// apply AoE attack
					list_t* aoeTargets = nullptr;
					list_t* shakeTargets = nullptr;
					Entity* tmpEntity = nullptr;
					if ( pose == MONSTER_POSE_GOLEM_SMASH && target == nullptr )
					{
						getTargetsAroundEntity(this, hit.entity, STRIKERANGE, PI / 3, MONSTER_TARGET_ENEMY, &aoeTargets);
						if ( aoeTargets )
						{
							for ( node = aoeTargets->first; node != NULL; node = node->next )
							{
								tmpEntity = (Entity*)node->element;
								if ( tmpEntity != nullptr )
								{
									this->attack(MONSTER_POSE_GOLEM_SMASH, charge, tmpEntity);
								}
							}
							//Free the list.
							list_FreeAll(aoeTargets);
							free(aoeTargets);
						}
						getTargetsAroundEntity(this, hit.entity, STRIKERANGE, PI, MONSTER_TARGET_PLAYER, &shakeTargets);
						if ( shakeTargets )
						{
							// shake nearby players that were not the primary target.
							for ( node = shakeTargets->first; node != NULL; node = node->next )
							{
								tmpEntity = (Entity*)node->element;
								playerhit = tmpEntity->skill[2];
								if ( playerhit > 0 && multiplayer == SERVER )
								{
									strcpy((char*)net_packet->data, "SHAK");
									net_packet->data[4] = 10; // turns into .1
									net_packet->data[5] = 10;
									net_packet->address.host = net_clients[playerhit - 1].host;
									net_packet->address.port = net_clients[playerhit - 1].port;
									net_packet->len = 6;
									sendPacketSafe(net_sock, -1, net_packet, playerhit - 1);
								}
								else if ( playerhit == 0 )
								{
									camera_shakex += 0.1;
									camera_shakey += 10;
								}
							}
							//Free the list.
							list_FreeAll(shakeTargets);
							free(shakeTargets);
						}
					}
					else if ( pose == MONSTER_POSE_AUTOMATON_MALFUNCTION )
					{
						getTargetsAroundEntity(this, this, 24, PI, MONSTER_TARGET_ALL, &aoeTargets);
						if ( aoeTargets )
						{
							for ( node = aoeTargets->first; node != NULL; node = node->next )
							{
								tmpEntity = (Entity*)node->element;
								if ( tmpEntity != nullptr )
								{
									spawnExplosion(tmpEntity->x, tmpEntity->y, tmpEntity->z);
									Stat* tmpStats = tmpEntity->getStats();
									if ( tmpStats )
									{
										int explodeDmg = (40 + myStats->HP) * damagetables[tmpStats->type][5]; // check base magic damage resist.
										Entity* gib = spawnGib(tmpEntity);
										serverSpawnGibForClient(gib);
										playerhit = tmpEntity->skill[2];
										if ( playerhit > 0 && multiplayer == SERVER )
										{
											strcpy((char*)net_packet->data, "SHAK");
											net_packet->data[4] = 20; // turns into .1
											net_packet->data[5] = 20;
											net_packet->address.host = net_clients[playerhit - 1].host;
											net_packet->address.port = net_clients[playerhit - 1].port;
											net_packet->len = 6;
											sendPacketSafe(net_sock, -1, net_packet, playerhit - 1);
										}
										else if ( playerhit == 0 )
										{
											camera_shakex += 0.2;
											camera_shakey += 20;
										}
										tmpEntity->modHP(-explodeDmg);
										if ( playerhit >= 0 )
										{
											Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
											messagePlayerColor(playerhit, color, language[2523]);
										}
									}
								}
							}
							//Free the list.
							list_FreeAll(aoeTargets);
							free(aoeTargets);
						}
					}
					// lifesteal
					if ( damage > 0 
						&& ((myStats->EFFECTS[EFF_VAMPIRICAURA] 
								&& (myStats->weapon == nullptr || myStats->type == LICH_FIRE)
							) 
							|| myStats->type == VAMPIRE) )
					{
						bool lifestealSuccess = false;
						if ( !wasBleeding && hitstats->EFFECTS[EFF_BLEEDING] )
						{
							// attack caused the target to bleed, trigger lifesteal tick
							this->modHP(damage);
							spawnMagicEffectParticles(x, y, z, 169);
							playSoundEntity(this, 168, 128);
							lifestealSuccess = true;
						}
						else if ( (rand() % 4 == 0) && (myStats->type == VAMPIRE && myStats->EFFECTS[EFF_VAMPIRICAURA]) )
						{
							// vampires under aura have higher chance.
							this->modHP(damage);
							spawnMagicEffectParticles(x, y, z, 169);
							playSoundEntity(this, 168, 128);
							lifestealSuccess = true;
						}
						else if ( rand() % 8 == 0 )
						{
							// else low chance for lifesteal.
							this->modHP(damage);
							spawnMagicEffectParticles(x, y, z, 169);
							playSoundEntity(this, 168, 128);
							lifestealSuccess = true;
						}

						if ( lifestealSuccess )
						{
							if ( player >= 0 )
							{
								myStats->HUNGER += 100;
							}
							if ( playerhit >= 0 )
							{
								Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
								messagePlayerColor(playerhit, color, language[2441]);
							}
							else
							{
								Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
								if ( !strcmp(hitstats->name, "") )
								{
									messagePlayerColor(player, color, language[2440], hit.entity->getMonsterLangEntry());
								}
								else
								{
									messagePlayerColor(player, color, language[2439], hitstats->name);
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
							bool degradePickaxe = true;
							if ( this->behavior == &actPlayer && MFLAG_DISABLEDIGGING )
							{
								Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 255);
								messagePlayerColor(this->skill[2], color, language[2380]); // disabled digging.
								playSoundPos(hit.x, hit.y, 66, 128); // strike wall
								// bang
								spawnBang(hit.x - cos(yaw) * 2, hit.y - sin(yaw) * 2, 0);
							}
							else if ( swimmingtiles[map.tiles[OBSTACLELAYER + hit.mapy * MAPLAYERS + hit.mapx * MAPLAYERS * map.height]]
								|| lavatiles[map.tiles[OBSTACLELAYER + hit.mapy * MAPLAYERS + hit.mapx * MAPLAYERS * map.height]] )
							{
								// no effect for lava/water tiles.
								degradePickaxe = false;
							}
							else
							{
								playSoundPos(hit.x, hit.y, 67, 128); // bust wall
								// spawn several rock items
								i = 8 + rand() % 4;
								for ( c = 0; c < i; c++ )
								{
									entity = newEntity(-1, 1, map.entities, nullptr); //Rock/item entity.
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

								if ( map.tiles[OBSTACLELAYER + hit.mapy * MAPLAYERS + hit.mapx * MAPLAYERS * map.height] >= 41
									&& map.tiles[OBSTACLELAYER + hit.mapy * MAPLAYERS + hit.mapx * MAPLAYERS * map.height] <= 49 )
								{
									steamAchievementClient(player, "BARONY_ACH_BAD_REVIEW");
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
								// Update the paths so that monsters know they can walk through it
								generatePathMaps();
							}

							if ( rand() % 2 && degradePickaxe )
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

			// apply AoE shake effect
			if ( pose == MONSTER_POSE_GOLEM_SMASH && target == nullptr )
			{
				list_t* shakeTargets = nullptr;
				Entity* tmpEntity = nullptr;
				getTargetsAroundEntity(this, hit.entity, STRIKERANGE, PI, MONSTER_TARGET_PLAYER, &shakeTargets);
				if ( shakeTargets )
				{
					// shake nearby players that were not the primary target.
					for ( node = shakeTargets->first; node != NULL; node = node->next )
					{
						tmpEntity = (Entity*)node->element;
						playerhit = tmpEntity->skill[2];
						if ( playerhit > 0 && multiplayer == SERVER )
						{
							strcpy((char*)net_packet->data, "SHAK");
							net_packet->data[4] = 10; // turns into .1
							net_packet->data[5] = 10;
							net_packet->address.host = net_clients[playerhit - 1].host;
							net_packet->address.port = net_clients[playerhit - 1].port;
							net_packet->len = 6;
							sendPacketSafe(net_sock, -1, net_packet, playerhit - 1);
						}
						else if ( playerhit == 0 )
						{
							camera_shakex += .1;
							camera_shakey += 10;
						}
					}
					//Free the list.
					list_FreeAll(shakeTargets);
					free(shakeTargets);
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
	if ( !stat )
	{
		return 0;
	}

	int armor = stat->CON;

	if ( stat->helmet )
	{
		armor += stat->helmet->armorGetAC();
	}
	if ( stat->breastplate )
	{
		armor += stat->breastplate->armorGetAC();
	}
	if ( stat->gloves )
	{
		armor += stat->gloves->armorGetAC();
	}
	if ( stat->shoes )
	{
		armor += stat->shoes->armorGetAC();
	}
	if ( stat->shield )
	{
		armor += stat->shield->armorGetAC();
	}
	if ( stat->cloak )
	{
		armor += stat->cloak->armorGetAC();
	}
	if ( stat->ring )
	{
		armor += stat->ring->armorGetAC();
	}

	if ( stat->shield )
	{
		int shieldskill = stat->PROFICIENCIES[PRO_SHIELD] / 25;
		armor += shieldskill;
		if ( stat->defending )
		{
			//messagePlayer(0, "shield up! +%d", 5 + stat->PROFICIENCIES[PRO_SHIELD] / 5);
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

bool Entity::teleport(int tele_x, int tele_y)
{
	int player = -1;

	if ( behavior == &actPlayer )
	{
		player = skill[2];
		if ( MFLAG_DISABLETELEPORT )
		{
			Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 255);
			// play sound effect
			playSoundEntity(this, 77, 64);
			messagePlayerColor(player, color, language[2381]);
			return false;
		}
	}

	if ( strstr(map.name, "Minotaur") || checkObstacle((tele_x << 4) + 8, (tele_y << 4) + 8, this, NULL) )
	{
		messagePlayer(player, language[707]);
		return false;
	}

	// play sound effect
	playSoundEntity(this, 77, 64);

	// relocate entity
	double oldx = x;
	double oldy = y;
	x = (tele_x << 4) + 8;
	y = (tele_y << 4) + 8;
	if ( entityInsideSomething(this) && getRace() != LICH_FIRE && getRace() != LICH_ICE )
	{
		x = oldx;
		y = oldy;
		if ( multiplayer == SERVER && player > 0 )
		{
			messagePlayer(player, language[707]);
		}
		return false;
	}
	if ( player > 0 && multiplayer == SERVER )
	{
		strcpy((char*)net_packet->data, "TELE");
		net_packet->data[4] = tele_x;
		net_packet->data[5] = tele_y;
		net_packet->address.host = net_clients[player - 1].host;
		net_packet->address.port = net_clients[player - 1].port;
		net_packet->len = 6;
		sendPacketSafe(net_sock, -1, net_packet, player - 1);
	}


	if ( behavior == actMonster )
	{
		if ( getRace() != LICH && getRace() != DEVIL && getRace() != LICH_FIRE && getRace() != LICH_ICE )
		{
			//messagePlayer(0, "Resetting monster's path after teleport.");
			monsterState = MONSTER_STATE_PATH;
			/*if ( children.first != nullptr )
			{
				list_RemoveNode(children.first);
			}*/
		}
	}

	// play second sound effect
	playSoundEntity(this, 77, 64);
	return true;
}

/*-------------------------------------------------------------------------------

Entity::teleportRandom

Teleports the given entity to a random location on the map.

-------------------------------------------------------------------------------*/

bool Entity::teleportRandom()
{
	int numlocations = 0;
	int pickedlocation;
	int player = -1;
	if ( behavior == &actPlayer )
	{
		player = skill[2];
		if ( MFLAG_DISABLETELEPORT )
		{
			Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 255);
			// play sound effect
			playSoundEntity(this, 77, 64);
			messagePlayerColor(player, color, language[2381]);
			return false;
		}

	}
	for ( int iy = 1; iy < map.height; ++iy )
	{
		for ( int ix = 1; ix < map.width; ++ix )
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
		return false;
	}
	pickedlocation = rand() % numlocations;
	numlocations = 0;
	for ( int iy = 1; iy < map.height; iy++ )
	{
		for ( int ix = 1; ix < map.width; ix++ )
		{
			if ( !checkObstacle((ix << 4) + 8, (iy << 4) + 8, this, NULL) )
			{
				if ( numlocations == pickedlocation )
				{
					teleport(ix, iy);
					return true;
				}
				numlocations++;
			}
		}
	}
	return false;
}

/*-------------------------------------------------------------------------------

Entity::teleportAroundEntity

Teleports the given entity within a radius of a target entity.

-------------------------------------------------------------------------------*/

bool Entity::teleportAroundEntity(const Entity* target, int dist)
{
	int numlocations = 0;
	int pickedlocation;
	int player = -1;
	int ty = static_cast<int>(std::floor(target->y)) >> 4;
	int tx = static_cast<int>(std::floor(target->x)) >> 4;

	if ( behavior == &actPlayer )
	{
		player = skill[2];
		if ( MFLAG_DISABLETELEPORT )
		{
			Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 255);
			// play sound effect
			playSoundEntity(this, 77, 64);
			messagePlayerColor(player, color, language[2381]);
			return false;
		}
	}
	for ( int iy = std::max(1, ty - dist); iy < std::min(ty + dist, static_cast<int>(map.height)); ++iy )
	{
		for ( int ix = std::max(1, tx - dist); ix < std::min(tx + dist, static_cast<int>(map.width)); ++ix )
		{
			if ( !checkObstacle((ix << 4) + 8, (iy << 4) + 8, this, NULL) )
			{
				numlocations++;
			}
		}
	}
	//messagePlayer(0, "locations: %d", numlocations);
	if ( numlocations == 0 )
	{
		messagePlayer(player, language[708]);
		return false;
	}
	pickedlocation = rand() % numlocations;
	numlocations = 0;
	for ( int iy = std::max(0, ty - dist); iy < std::min(ty + dist, static_cast<int>(map.height)); ++iy )
	{
		for ( int ix = std::max(0, tx - dist); ix < std::min(tx + dist, static_cast<int>(map.width)); ++ix )
		{
			if ( !checkObstacle((ix << 4) + 8, (iy << 4) + 8, this, NULL) )
			{
				if ( numlocations == pickedlocation )
				{
					return teleport(ix, iy);
				}
				numlocations++;
			}
		}
	}
	return false;
}

/*-------------------------------------------------------------------------------

Entity::teleporterMove

Teleports the given entity to the given (x, y) location on the map,
in map coordinates. Will not teleport if the destination is an obstacle.

-------------------------------------------------------------------------------*/

bool Entity::teleporterMove(int tele_x, int tele_y, int type)
{
	int player = -1;

	if ( behavior == &actPlayer )
	{
		player = skill[2];
	}
	// Can be inside entities?
	//if ( strstr(map.name, "Minotaur") || checkObstacle((tele_x << 4) + 8, (tele_y << 4) + 8, this, NULL) )
	//{
	//	messagePlayer(player, language[707]);
	//	return false;
	//}

	// relocate entity
	double oldx = x;
	double oldy = y;
	x = (tele_x << 4) + 8;
	y = (tele_y << 4) + 8;
	/*if ( entityInsideSomething(this) )
	{
		x = oldx;
		y = oldy;
		if ( multiplayer == SERVER && player > 0 )
		{
			messagePlayer(player, language[707]);
		}
		return false;
	}*/
	if ( player > 0 && multiplayer == SERVER )
	{
		strcpy((char*)net_packet->data, "TELM");
		net_packet->data[4] = tele_x;
		net_packet->data[5] = tele_y;
		net_packet->data[6] = type;
		net_packet->address.host = net_clients[player - 1].host;
		net_packet->address.port = net_clients[player - 1].port;
		net_packet->len = 7;
		sendPacketSafe(net_sock, -1, net_packet, player - 1);
	}

	// play sound effect
	if ( type == 0 || type == 1 )
	{
		playSoundEntityLocal(this, 96, 64);
	}
	else if ( type == 2 )
	{
		playSoundEntityLocal(this, 154, 64);
	}
	return true;
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

	if ( src->monsterAllySummonRank != 0 )
	{
		return; // summoned monster, no XP!
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

		for ( c = 0; c < MAXPLAYERS; ++c )
		{
			shares[c] = nullptr;
		}

		// find other players to divide shares with
		node_t* node;
		for ( node = map.creatures->first; node != nullptr; node = node->next ) //Since only looking at players, this should just iterate over players[]
		{
			Entity* entity = (Entity*)node->element;
			if ( entity == this )
			{
				continue;
			}
			if ( entity && entity->behavior == &actPlayer )
			{
				if ( entityDist(this, entity) < XPSHARERANGE )
				{
					++numshares;
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

			if ( this->behavior == &actPlayer )
			{
				if ( stats[this->skill[2]] )
				{
					// award XP to player's followers.
					int numFollowers = list_Size(&stats[this->skill[2]]->FOLLOWERS);
					for ( node = stats[this->skill[2]]->FOLLOWERS.first; node != nullptr; node = node->next )
					{
						Entity* follower = uidToEntity(*((Uint32*)node->element));
						if ( entityDist(this, follower) < XPSHARERANGE && follower != src )
						{
							Stat* followerStats = follower->getStats();
							if ( followerStats )
							{
								int xpDivide = std::min(std::max(1, numFollowers), 4); // 1 - 4 depending on followers.
								followerStats->EXP += (xpGain / xpDivide);
								//messagePlayer(0, "monster got %d xp", xpGain);
							}
						}
					}
				}
			}
		}
		
	}

	// award XP to main victor
	destStats->EXP += xpGain;

	if ( (srcStats->type == LICH || srcStats->type == LICH_FIRE || srcStats->type == LICH_ICE) && root )
	{
		if ( destStats->type == CREATURE_IMP 
			|| destStats->type == DEMON
			|| (destStats->type == AUTOMATON && !strcmp(destStats->name, "corrupted automaton")) )
		{
			if ( !flags[USERFLAG2] )
			{
				for ( int c = 0; c < MAXPLAYERS; c++ )
				{
					steamAchievementClient(c, "BARONY_ACH_OWN_WORST_ENEMY");
				}
			}
		}
	}

	// award bonus XP and update kill counters
	if ( player >= 0 )
	{
		if ( root == false )
		{
			updateAchievementThankTheTank(player, src, true);
		}
		if ( currentlevel >= 25 && srcStats->type == MINOTAUR )
		{
			for ( int c = 0; c < MAXPLAYERS; c++ )
			{
				steamAchievementClient(c, "BARONY_ACH_REUNITED");
			}
		}
		if ( srcStats->type == SHADOW && root )
		{
			std::string name = "Shadow of ";
			name += stats[player]->name;
			if ( name.compare(srcStats->name) == 0 )
			{
				steamAchievementClient(player, "BARONY_ACH_KNOW_THYSELF");
			}
		}
		if ( srcStats->LVL >= 25 && root 
			&& destStats->HP <= 5 && checkEnemy(src) )
		{
			steamAchievementClient(player, "BARONY_ACH_BUT_A_SCRATCH");
		}
		if ( srcStats->EFFECTS[EFF_PARALYZED] )
		{
			serverUpdatePlayerGameplayStats(player, STATISTICS_SITTING_DUCK, 1);
		}

		if ( player == 0 )
		{
			if ( srcStats->type == LICH )
			{
				kills[LICH] = 1;
			}
			else if ( srcStats->type == LICH_FIRE )
			{
				kills[LICH]++;
			}
			else if ( srcStats->type == LICH_ICE )
			{
				kills[LICH]++;
			}
			else
			{
				kills[srcStats->type]++;
			}
		}
		else if ( multiplayer == SERVER && player > 0 )
		{
			// inform client of kill
			strcpy((char*)net_packet->data, "MKIL");
			if ( srcStats->type == LICH_FIRE || srcStats->type == LICH_ICE )
			{
				net_packet->data[4] = LICH;
			}
			else
			{
				net_packet->data[4] = srcStats->type;
			}
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
	if ( !your )
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

	if ( myStats->type == HUMAN && (yourStats->type == AUTOMATON && !strncmp(yourStats->name, "corrupted automaton", 19)) )
	{
		return true;
	}
	else if ( (yourStats->type == HUMAN || your->behavior == &actPlayer) && (myStats->type == AUTOMATON && !strncmp(myStats->name, "corrupted automaton", 19)) )
	{
		return true;
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

			// player exceptions to table go here.
			if ( behavior == &actPlayer && myStats->type != HUMAN )
			{
				result = swornenemies[HUMAN][yourStats->type];
				switch ( myStats->type )
				{
					case SKELETON:
						if ( yourStats->type == GHOUL )
						{
							result = false;
						}
						break;
					case GOBLIN:
						if ( yourStats->type == GOBLIN )
						{
							result = false;
						}
						break;
					case GOATMAN:
						if ( yourStats->type == GOATMAN )
						{
							result = false;
						}
						break;
					case INCUBUS:
					case SUCCUBUS:
						if ( yourStats->type == SUCCUBUS || yourStats->type == INCUBUS )
						{
							result = false;
						}
						break;
					case INSECTOID:
						if ( yourStats->type == SCARAB || yourStats->type == SPIDER || yourStats->type == INSECTOID )
						{
							result = false;
						}
						break;
					case VAMPIRE:
						if ( yourStats->type == VAMPIRE )
						{
							result = false;
						}
						break;
					case AUTOMATON:
						if ( yourStats->type == KOBOLD )
						{
							result = false;
						}
						break;
					default:
						break;
				}
			}
			else if ( behavior == &actMonster && your->behavior == &actPlayer && yourStats->type != HUMAN )
			{
				result = swornenemies[myStats->type][HUMAN];
				if ( myStats->type == HUMAN || myStats->type == SHOPKEEPER )
				{
					// enemies.
					result = true;
				}
				else
				{
					switch ( yourStats->type )
					{
						case SKELETON:
							if ( myStats->type == GHOUL )
							{
								result = false;
							}
							break;
						case GOBLIN:
							if ( myStats->type == GOBLIN )
							{
								result = false;
							}
							break;
						case GOATMAN:
							if ( myStats->type == GOATMAN )
							{
								result = false;
							}
							break;
						case INCUBUS:
						case SUCCUBUS:
							if ( myStats->type == SUCCUBUS || myStats->type == INCUBUS )
							{
								result = false;
							}
							break;
						case INSECTOID:
							if ( myStats->type == SCARAB || myStats->type == SPIDER || myStats->type == INSECTOID )
							{
								result = false;
							}
							break;
						case VAMPIRE:
							if ( myStats->type == VAMPIRE )
							{
								result = false;
							}
							break;
						case AUTOMATON:
							if ( myStats->type == KOBOLD )
							{
								result = false;
							}
							break;
						default:
							break;
					}
				}
			}
		}
	}

	// confused monsters mistake their allegiances
	if ( myStats->EFFECTS[EFF_CONFUSED] )
	{
		if ( myStats->type == AUTOMATON && yourStats->type == AUTOMATON 
			&& !strncmp(myStats->name, "corrupted automaton", strlen("corrupted automaton")) )
		{
			// these guys ignore themselves when confused..
		}
		else
		{
			result = (result == false);
		}
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

	if ( !your )
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

	if ( (myStats->type == HUMAN || behavior == &actPlayer) && (yourStats->type == AUTOMATON && !strncmp(yourStats->name, "corrupted automaton", 19)) )
	{
		return false;
	}
	else if ( (yourStats->type == HUMAN || your->behavior == &actPlayer) && (myStats->type == AUTOMATON && !strncmp(myStats->name, "corrupted automaton", 19)) )
	{
		return false;
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

			// player exceptions to table go here.
			if ( behavior == &actPlayer && myStats->type != HUMAN )
			{
				result = monsterally[HUMAN][yourStats->type];
				if ( myStats->type == HUMAN || myStats->type == SHOPKEEPER )
				{
					result = false;
				}
				else
				{
					switch ( myStats->type )
					{
						case SKELETON:
							if ( yourStats->type == GHOUL )
							{
								result = true;
							}
							break;
						case GOBLIN:
							if ( yourStats->type == GOBLIN )
							{
								result = true;
							}
							break;
						case GOATMAN:
							if ( yourStats->type == GOATMAN )
							{
								result = true;
							}
							break;
						case INCUBUS:
						case SUCCUBUS:
							if ( yourStats->type == SUCCUBUS || yourStats->type == INCUBUS )
							{
								result = true;
							}
							break;
						case INSECTOID:
							if ( yourStats->type == SCARAB || yourStats->type == SPIDER || yourStats->type == INSECTOID )
							{
								result = true;
							}
							break;
						case VAMPIRE:
							if ( yourStats->type == VAMPIRE )
							{
								result = true;
							}
							break;
						case AUTOMATON:
							if ( yourStats->type == KOBOLD )
							{
								result = true;
							}
							break;
						default:
							break;
					}
				}
			}
			else if ( behavior == &actMonster && your->behavior == &actPlayer && yourStats->type != HUMAN )
			{
				result = monsterally[myStats->type][HUMAN];
				if ( myStats->type == HUMAN || myStats->type == SHOPKEEPER )
				{
					result = false;
				}
				else
				{
					switch ( yourStats->type )
					{
						case SKELETON:
							if ( myStats->type == GHOUL )
							{
								result = true;
							}
							break;
						case GOBLIN:
							if ( myStats->type == GOBLIN )
							{
								result = true;
							}
							break;
						case GOATMAN:
							if ( myStats->type == GOATMAN )
							{
								result = true;
							}
							break;
						case INCUBUS:
						case SUCCUBUS:
							if ( myStats->type == SUCCUBUS || myStats->type == INCUBUS )
							{
								result = true;
							}
							break;
						case INSECTOID:
							if ( myStats->type == SCARAB || myStats->type == SPIDER || myStats->type == INSECTOID )
							{
								result = true;
							}
							break;
						case VAMPIRE:
							if ( myStats->type == VAMPIRE )
							{
								result = true;
							}
							break;
						case AUTOMATON:
							if ( myStats->type == KOBOLD )
							{
								result = true;
							}
							break;
						default:
							break;
					}
				}
			}
		}
	}

	return result;
}


void createMonsterEquipment(Stat* stats)
{
	int itemIndex = 0;
	ItemType itemId;
	Status itemStatus;
	int itemBless;
	int itemAppearance = rand();
	int itemCount;
	int chance = 1;
	int category = 0;
	bool itemIdentified;
	if ( stats != nullptr )
	{
		for ( itemIndex = 0; itemIndex < 10; ++itemIndex )
		{
			bool generateItem = true;
			category = stats->EDITOR_ITEMS[itemIndex * ITEM_SLOT_NUMPROPERTIES + ITEM_SLOT_CATEGORY];
			if ( category > 0 && stats->EDITOR_ITEMS[itemIndex * ITEM_SLOT_NUMPROPERTIES] == 1 )
			{
				if ( category > 0 && category <= 13 )
				{
					itemId = itemLevelCurve(static_cast<Category>(category - 1), 0, currentlevel);
				}
				else
				{
					int randType = 0;
					if ( category == 14 )
					{
						// equipment
						randType = rand() % 2;
						if ( randType == 0 )
						{
							itemId = itemLevelCurve(static_cast<Category>(WEAPON), 0, currentlevel);
						}
						else if ( randType == 1 )
						{
							itemId = itemLevelCurve(static_cast<Category>(ARMOR), 0, currentlevel);
						}
					}
					else if ( category == 15 )
					{
						// jewelry
						randType = rand() % 2;
						if ( randType == 0 )
						{
							itemId = itemLevelCurve(static_cast<Category>(AMULET), 0, currentlevel);
						}
						else
						{
							itemId = itemLevelCurve(static_cast<Category>(RING), 0, currentlevel);
						}
					}
					else if ( category == 16 )
					{
						// magical
						randType = rand() % 3;
						if ( randType == 0 )
						{
							itemId = itemLevelCurve(static_cast<Category>(SCROLL), 0, currentlevel);
						}
						else if ( randType == 1 )
						{
							itemId = itemLevelCurve(static_cast<Category>(MAGICSTAFF), 0, currentlevel);
						}
						else
						{
							itemId = itemLevelCurve(static_cast<Category>(SPELLBOOK), 0, currentlevel);
						}
					}
				}
			}
			else
			{
				if ( static_cast<ItemType>(stats->EDITOR_ITEMS[itemIndex * ITEM_SLOT_NUMPROPERTIES] - 2) >= 0 )
				{
					itemId = static_cast<ItemType>(stats->EDITOR_ITEMS[itemIndex * ITEM_SLOT_NUMPROPERTIES] - 2);
				}
				else
				{
					itemId = ItemType::WOODEN_SHIELD;
					generateItem = false;
				}
			}

			if ( itemId >= 0 && generateItem )
			{
				itemStatus = static_cast<Status>(stats->EDITOR_ITEMS[itemIndex * ITEM_SLOT_NUMPROPERTIES + 1]);
				if ( itemStatus == 0 )
				{
					itemStatus = static_cast<Status>(DECREPIT + rand() % 4);
				}
				else if ( itemStatus > BROKEN )
				{
					itemStatus = static_cast<Status>(itemStatus - 1); // reserved '0' for random, so '1' is decrepit... etc to '5' being excellent.
				}
				itemBless = stats->EDITOR_ITEMS[itemIndex * ITEM_SLOT_NUMPROPERTIES + 2];
				if ( itemBless == 10 )
				{
					itemBless = -2 + rand() % 5;
				}
				itemCount = stats->EDITOR_ITEMS[itemIndex * ITEM_SLOT_NUMPROPERTIES + 3];
				if ( stats->EDITOR_ITEMS[itemIndex * ITEM_SLOT_NUMPROPERTIES + 4] == 1 )
				{
					itemIdentified = false;
				}
				else if ( stats->EDITOR_ITEMS[itemIndex * ITEM_SLOT_NUMPROPERTIES + 4] == 2 )
				{
					itemIdentified = true;
				}
				else
				{
					itemIdentified = rand() % 2;
				}
				itemAppearance = rand();
				chance = stats->EDITOR_ITEMS[itemIndex * ITEM_SLOT_NUMPROPERTIES + 5];

				if ( rand() % 100 < chance )
				{
					switch ( itemIndex ) {
						case 0:
							stats->helmet = newItem(itemId, itemStatus, itemBless, itemCount, itemAppearance, itemIdentified, NULL);
							break;
						case 1:
							stats->weapon = newItem(itemId, itemStatus, itemBless, itemCount, itemAppearance, itemIdentified, NULL);
							break;
						case 2:
							stats->shield = newItem(itemId, itemStatus, itemBless, itemCount, itemAppearance, itemIdentified, NULL);
							break;
						case 3:
							stats->breastplate = newItem(itemId, itemStatus, itemBless, itemCount, itemAppearance, itemIdentified, NULL);
							break;
						case 4:
							stats->shoes = newItem(itemId, itemStatus, itemBless, itemCount, itemAppearance, itemIdentified, NULL);
							break;
						case 5:
							stats->ring = newItem(itemId, itemStatus, itemBless, itemCount, itemAppearance, itemIdentified, NULL);
							break;
						case 6:
							stats->amulet = newItem(itemId, itemStatus, itemBless, itemCount, itemAppearance, itemIdentified, NULL);
							break;
						case 7:
							stats->cloak = newItem(itemId, itemStatus, itemBless, itemCount, itemAppearance, itemIdentified, NULL);
							break;
						case 8:
							stats->mask = newItem(itemId, itemStatus, itemBless, itemCount, itemAppearance, itemIdentified, NULL);
							break;
						case 9:
							stats->gloves = newItem(itemId, itemStatus, itemBless, itemCount, itemAppearance, itemIdentified, NULL);
							break;
						default:
							break;
					}
				}
			}
		}
	}
}

int countCustomItems(Stat* stats)
{
	int x = 0;
	int customItemSlotCount = 0;

	for ( x = ITEM_SLOT_INV_1; x <= ITEM_SLOT_INV_6; x = x + ITEM_SLOT_NUMPROPERTIES )
	{
		if ( stats->EDITOR_ITEMS[x] != 1 || (stats->EDITOR_ITEMS[x] == 1 && stats->EDITOR_ITEMS[x + ITEM_SLOT_CATEGORY] != 0) )
		{
			++customItemSlotCount; //found a custom item in inventory
		}
	}

	return customItemSlotCount; //use custom items from editor instead of default generation
}

int countDefaultItems(Stat* stats)
{
	int x = 0;
	int defaultItemSlotCount = 0;

	for ( x = ITEM_SLOT_INV_1; x <= ITEM_SLOT_INV_6; x = x + ITEM_SLOT_NUMPROPERTIES )
	{
		if ( stats->EDITOR_ITEMS[x] == 1 && stats->EDITOR_ITEMS[x + ITEM_SLOT_CATEGORY] == 0 )
		{
			defaultItemSlotCount++; //found a default item in inventory
		}
	}

	return defaultItemSlotCount;
}

void setRandomMonsterStats(Stat* stats)
{
	if ( stats != nullptr )
	{
		//**************************************
		// HEALTH
		//**************************************

		if ( stats->MAXHP == stats->HP )
		{
			stats->MAXHP += rand() % (stats->RANDOM_MAXHP + 1);

			if ( stats->RANDOM_MAXHP == stats->RANDOM_HP )
			{
				// if the max hp and normal hp range is the same, hp follows the roll of maxhp.
				stats->HP = stats->MAXHP;
			}
			else
			{
				// roll the current hp
				stats->HP += rand() % (stats->RANDOM_HP + 1);
			}
		}
		else
		{
			// roll both ranges independently
			stats->MAXHP += rand() % (stats->RANDOM_MAXHP + 1);
			stats->HP += rand() % (stats->RANDOM_HP + 1);
		}

		if ( stats->HP > stats->MAXHP )
		{
			// check if hp exceeds maximums
			stats->HP = stats->MAXHP;
		}
		stats->OLDHP = stats->HP;

		//**************************************
		// MANA
		//**************************************

		if ( stats->MAXMP == stats->MP )
		{
			stats->MAXMP += rand() % (stats->RANDOM_MAXMP + 1);

			if ( stats->RANDOM_MAXMP == stats->RANDOM_MP )
			{
				// if the max mp and normal mp range is the same, mp follows the roll of maxmp.
				stats->MP = stats->MAXMP;
			}
			else
			{
				// roll the current mp
				stats->MP += rand() % (stats->RANDOM_MP + 1);
			}
		}
		else
		{
			// roll both ranges independently
			stats->MAXMP += rand() % (stats->RANDOM_MAXMP + 1);
			stats->MP += rand() % (stats->RANDOM_MP + 1);
		}

		if ( stats->MP > stats->MAXMP )
		{
			// check if mp exceeds maximums
			stats->MP = stats->MAXMP;
		}

		//**************************************
		// REST OF STATS
		//**************************************

		stats->STR += rand() % (stats->RANDOM_STR + 1);
		stats->DEX += rand() % (stats->RANDOM_DEX + 1);
		stats->CON += rand() % (stats->RANDOM_CON + 1);
		stats->INT += rand() % (stats->RANDOM_INT + 1);
		stats->PER += rand() % (stats->RANDOM_PER + 1);
		stats->CHR += rand() % (stats->RANDOM_CHR + 1);

		stats->LVL += rand() % (stats->RANDOM_LVL + 1);
		stats->GOLD += rand() % (stats->RANDOM_GOLD + 1);
	}

	// debug print out each monster spawned

	/*messagePlayer(0, "Set stats to: ");
	messagePlayer(0, "MAXHP: %d", stats->MAXHP);
	messagePlayer(0, "HP: %d", stats->HP);
	messagePlayer(0, "MAXMP: %d", stats->MAXMP);
	messagePlayer(0, "MP: %d", stats->MP);
	messagePlayer(0, "Str: %d", stats->STR);
	messagePlayer(0, "Dex: %d", stats->DEX);
	messagePlayer(0, "Con: %d", stats->CON);
	messagePlayer(0, "Int: %d", stats->INT);
	messagePlayer(0, "Per: %d", stats->PER);
	messagePlayer(0, "Chr: %d", stats->CHR);
	messagePlayer(0, "LVL: %d", stats->LVL);
	messagePlayer(0, "GOLD: %d", stats->GOLD);*/


	return;
}


int checkEquipType(const Item *item)
{
	switch ( item->type ) {

		case LEATHER_BOOTS:
		case LEATHER_BOOTS_SPEED:
		case IRON_BOOTS:
		case IRON_BOOTS_WATERWALKING:
		case STEEL_BOOTS:
		case STEEL_BOOTS_LEVITATION:
		case STEEL_BOOTS_FEATHER:
		case CRYSTAL_BOOTS:
		case ARTIFACT_BOOTS:
			return TYPE_BOOTS;
			break;

		case LEATHER_HELM:
		case IRON_HELM:
		case STEEL_HELM:
		case CRYSTAL_HELM:
		case ARTIFACT_HELM:
			return TYPE_HELM;
			break;

		case LEATHER_BREASTPIECE:
		case IRON_BREASTPIECE:
		case STEEL_BREASTPIECE:
		case CRYSTAL_BREASTPIECE:
		case WIZARD_DOUBLET:
		case HEALER_DOUBLET:
		case VAMPIRE_DOUBLET:
		case ARTIFACT_BREASTPIECE:
			return TYPE_BREASTPIECE;
			break;

		case CRYSTAL_SHIELD:
		case WOODEN_SHIELD:
		case BRONZE_SHIELD:
		case IRON_SHIELD:
		case STEEL_SHIELD:
		case STEEL_SHIELD_RESISTANCE:
		case MIRROR_SHIELD:
			return TYPE_SHIELD;
			break;

		case TOOL_TORCH:
		case TOOL_LANTERN:
		case TOOL_CRYSTALSHARD:
			return TYPE_OFFHAND;
			break;

		case CLOAK:
		case CLOAK_MAGICREFLECTION:
		case CLOAK_INVISIBILITY:
		case CLOAK_PROTECTION:
		case ARTIFACT_CLOAK:
		case CLOAK_BLACK:
			return TYPE_CLOAK;
			break;

		case GLOVES:
		case GLOVES_DEXTERITY:
		case GAUNTLETS:
		case GAUNTLETS_STRENGTH:
		case BRACERS:
		case BRACERS_CONSTITUTION:
		case CRYSTAL_GLOVES:
		case ARTIFACT_GLOVES:
		case SPIKED_GAUNTLETS:
		case IRON_KNUCKLES:
		case BRASS_KNUCKLES:
			return TYPE_GLOVES;
			break;

		case HAT_HOOD:
		case HAT_JESTER:
		case HAT_PHRYGIAN:
		case HAT_WIZARD:
		case HAT_FEZ:
			return TYPE_HAT;
			break;

		default:
			break;
	}

	return TYPE_NONE;
}

int setGloveSprite(Stat* myStats, Entity* ent, int spriteOffset)
{
	if ( myStats == nullptr )
	{
		return 0;
	}
	if ( myStats->gloves == nullptr )
	{
		return 0;
	}

	if ( myStats->gloves->type == GLOVES || myStats->gloves->type == GLOVES_DEXTERITY ) {
		ent->sprite = 132 + myStats->sex + spriteOffset;
	}
	else if ( myStats->gloves->type == BRACERS || myStats->gloves->type == BRACERS_CONSTITUTION ) {
		ent->sprite = 323 + myStats->sex + spriteOffset;
	}
	else if ( myStats->gloves->type == GAUNTLETS || myStats->gloves->type == GAUNTLETS_STRENGTH ) {
		ent->sprite = 140 + myStats->sex + spriteOffset;
	}
	else if ( myStats->gloves->type == CRYSTAL_GLOVES )
	{
		ent->sprite = 491 + myStats->sex + spriteOffset;
	}
	else if ( myStats->gloves->type == ARTIFACT_GLOVES )
	{
		ent->sprite = 513 + myStats->sex + spriteOffset;
	}
	else if ( myStats->gloves->type == BRASS_KNUCKLES )
	{
		ent->sprite = 531 + myStats->sex + spriteOffset;
	}
	else if ( myStats->gloves->type == IRON_KNUCKLES )
	{
		ent->sprite = 539 + myStats->sex + spriteOffset;
	}
	else if ( myStats->gloves->type == SPIKED_GAUNTLETS )
	{
		ent->sprite = 547 + myStats->sex + spriteOffset;
	}
	else
	{
		return 0;
	}
	return 1;
}

bool Entity::setBootSprite(Entity* leg, int spriteOffset)
{
	if ( multiplayer == CLIENT )
	{
		return false;
	}

	Stat* myStats;

	if ( this->behavior == &actPlayer )
	{
		myStats = stats[this->skill[2]]; // skill[2] contains the player number.
	}
	else
	{
		myStats = this->getStats();
	}

	if ( myStats == nullptr )
	{
		return false;
	}
	if ( myStats->shoes == nullptr )
	{
		return false;
	}

	switch ( myStats->type )
	{
		case HUMAN:
			if ( myStats->shoes->type == LEATHER_BOOTS || myStats->shoes->type == LEATHER_BOOTS_SPEED )
			{
				leg->sprite = 148 + myStats->sex + spriteOffset;
			}
			else if ( myStats->shoes->type == IRON_BOOTS || myStats->shoes->type == IRON_BOOTS_WATERWALKING )
			{
				leg->sprite = 152 + myStats->sex + spriteOffset;
			}
			else if ( myStats->shoes->type >= STEEL_BOOTS && myStats->shoes->type <= STEEL_BOOTS_FEATHER )
			{
				leg->sprite = 156 + myStats->sex + spriteOffset;
			}
			else if ( myStats->shoes->type == CRYSTAL_BOOTS )
			{
				leg->sprite = 499 + myStats->sex + spriteOffset;
			}
			else if ( myStats->shoes->type == ARTIFACT_BOOTS )
			{
				leg->sprite = 521 + myStats->sex + spriteOffset;
			}
			else
			{
				return false;
			}
			break;
			// fall throughs below
		case AUTOMATON:
		case GOATMAN:
		case INSECTOID:
		case KOBOLD:
		case GOBLIN:
		case SKELETON:
		case GNOME:
		case SHADOW:
		case INCUBUS:
		case VAMPIRE:
		case SUCCUBUS:
		case SHOPKEEPER:
			if ( myStats->shoes->type == LEATHER_BOOTS || myStats->shoes->type == LEATHER_BOOTS_SPEED )
			{
				leg->sprite = 148 + spriteOffset;
			}
			else if ( myStats->shoes->type == IRON_BOOTS || myStats->shoes->type == IRON_BOOTS_WATERWALKING )
			{
				leg->sprite = 152 + spriteOffset;
			}
			else if ( myStats->shoes->type >= STEEL_BOOTS && myStats->shoes->type <= STEEL_BOOTS_FEATHER )
			{
				leg->sprite = 156 + spriteOffset;
			}
			else if ( myStats->shoes->type == CRYSTAL_BOOTS )
			{
				leg->sprite = 499 + spriteOffset;
			}
			else if ( myStats->shoes->type == ARTIFACT_BOOTS )
			{
				leg->sprite = 521 + spriteOffset;
			}
			else
			{
				return false;
			}
			break;
		default:
			break;
	}

	return true;
}


/*-------------------------------------------------------------------------------

sLevitating

returns true if the given entity is levitating, or false if it cannot

-------------------------------------------------------------------------------*/

bool isLevitating(Stat* mystats)
{
	if ( mystats == nullptr )
	{
		return false;
	}

	// check levitating value
	bool levitating = false;
	if ( MFLAG_DISABLELEVITATION )
	{
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			if ( client_disconnected[i] )
			{
				continue;
			}
			// check if mystats is a player, and levitation flag is disabled.
			if ( players[i] && players[i]->entity )
			{
				if ( players[i]->entity->getStats() == mystats )
				{
					return false;
				}
			}
		}
	}
	if ( mystats->EFFECTS[EFF_LEVITATING] == true )
	{
		return true;
	}
	if ( mystats->ring != NULL )
	{
		if ( mystats->ring->type == RING_LEVITATION )
		{
			return true;
		}
	}
	if ( mystats->shoes != NULL )
	{
		if ( mystats->shoes->type == STEEL_BOOTS_LEVITATION )
		{
			return true;
		}
	}
	if ( mystats->cloak != NULL )
	{
		if ( mystats->cloak->type == ARTIFACT_CLOAK )
		{
			return true;
		}
	}

	return false;
}

/*-------------------------------------------------------------------------------

getWeaponSkill

returns the proficiency for the weapon equipped.

-------------------------------------------------------------------------------*/

int getWeaponSkill(Item* weapon)
{
	if ( weapon == NULL )
	{
		return -1;
	}

	if ( weapon->type == QUARTERSTAFF || weapon->type == IRON_SPEAR || weapon->type == STEEL_HALBERD || weapon->type == ARTIFACT_SPEAR || weapon->type == CRYSTAL_SPEAR )
	{
		return PRO_POLEARM;
	}
	if ( weapon->type == BRONZE_SWORD || weapon->type == IRON_SWORD || weapon->type == STEEL_SWORD || weapon->type == ARTIFACT_SWORD || weapon->type == CRYSTAL_SWORD )
	{
		return PRO_SWORD;
	}
	if ( weapon->type == BRONZE_MACE || weapon->type == IRON_MACE || weapon->type == STEEL_MACE || weapon->type == ARTIFACT_MACE || weapon->type == CRYSTAL_MACE )
	{
		return PRO_MACE;
	}
	if ( weapon->type == BRONZE_AXE || weapon->type == IRON_AXE || weapon->type == STEEL_AXE || weapon->type == ARTIFACT_AXE || weapon->type == CRYSTAL_BATTLEAXE )
	{
		return PRO_AXE;
	}
	if ( weapon->type == SLING || weapon->type == SHORTBOW || weapon->type == CROSSBOW || weapon->type == ARTIFACT_BOW )
	{
		return PRO_RANGED;
	}
	if ( itemCategory(weapon) == THROWN || itemCategory(weapon) == POTION || itemCategory(weapon) == GEM )
	{
		return PRO_RANGED;
	}
	return -1;
}

/*-------------------------------------------------------------------------------

getStatForProficiency

returns the stat associated with the given proficiency.

-------------------------------------------------------------------------------*/

int getStatForProficiency(int skill)
{
	int statForProficiency = -1;

	switch ( skill )
	{
		case PRO_SWORD:			// base attribute: str
		case PRO_MACE:			// base attribute: str
		case PRO_AXE:			// base attribute: str
		case PRO_POLEARM:		// base attribute: str
			statForProficiency = STAT_STR;
			break;
		case PRO_LOCKPICKING:	// base attribute: dex
		case PRO_STEALTH:		// base attribute: dex
		case PRO_RANGED:        // base attribute: dex
			statForProficiency = STAT_DEX;
			break;
		case PRO_SWIMMING:      // base attribute: con
		case PRO_SHIELD:		// base attribute: con
			statForProficiency = STAT_CON;
			break;
		case PRO_SPELLCASTING:  // base attribute: int
		case PRO_MAGIC:         // base attribute: int
			statForProficiency = STAT_INT;
			break;
		case PRO_APPRAISAL:		// base attribute: per
			statForProficiency = STAT_PER;
			break;
		case PRO_TRADING:       // base attribute: chr
		case PRO_LEADERSHIP:    // base attribute: chr
			statForProficiency = STAT_CHR;
			break;
		default:
			statForProficiency = -1;
			break;
	}

	return statForProficiency;
}


int Entity::isEntityPlayer() const
{
	for ( int i = 0; i < numplayers; ++i )
	{
		if ( this == players[i]->entity )
		{
			return i;
		}
	}

	return -1;
}

int Entity::getReflection() const
{
	Stat *stats = getStats();
	if ( !stats )
	{
		return 0;
	}

	if ( stats->EFFECTS[EFF_MAGICREFLECT] )
	{
		return 3;
	}

	if ( stats->shield )
	{
		if ( stats->shield->type == MIRROR_SHIELD && stats->defending )
		{
			return 3;
		}
	}
	if ( stats->amulet )
	{
		if ( stats->amulet->type == AMULET_MAGICREFLECTION )
		{
			return 2;
		}
	}
	if ( stats->cloak )
	{
		if ( stats->cloak->type == CLOAK_MAGICREFLECTION )
		{
			return 1;
		}
	}
	return 0;
}

int Entity::getAttackPose() const
{
	Stat *myStats = getStats();
	if ( !myStats )
	{
		return -1;
	}

	int pose = 0;

	if ( myStats->weapon != nullptr )
	{
		if ( myStats->type == LICH_FIRE )
		{
			switch ( monsterLichFireMeleeSeq )
			{
				case LICH_ATK_VERTICAL_SINGLE:
					pose = MONSTER_POSE_MELEE_WINDUP1;
					break;
				case LICH_ATK_HORIZONTAL_SINGLE:
					pose = MONSTER_POSE_MELEE_WINDUP2;
					break;
				case LICH_ATK_RISING_RAIN:
					pose = MONSTER_POSE_SPECIAL_WINDUP1;
					break;
				case LICH_ATK_BASICSPELL_SINGLE:
					pose = MONSTER_POSE_MAGIC_WINDUP1;
					break;
				case LICH_ATK_RISING_SINGLE:
					pose = MONSTER_POSE_MELEE_WINDUP3;
					break;
				case LICH_ATK_VERTICAL_QUICK:
					pose = MONSTER_POSE_MELEE_WINDUP1;
					break;
				case LICH_ATK_HORIZONTAL_RETURN:
					pose = MONSTER_POSE_MELEE_WINDUP2;
					break;
				case LICH_ATK_HORIZONTAL_QUICK:
					pose = MONSTER_POSE_MELEE_WINDUP2;
					break;
				case LICH_ATK_SUMMON:
					pose = MONSTER_POSE_MAGIC_WINDUP3;
					break;
				default:
					break;
			}
		}
		else if ( myStats->type == LICH_ICE )
		{
			switch ( monsterLichIceCastSeq )
			{
				case LICH_ATK_VERTICAL_SINGLE:
					pose = MONSTER_POSE_MELEE_WINDUP1;
					break;
				case LICH_ATK_HORIZONTAL_SINGLE:
					pose = MONSTER_POSE_MELEE_WINDUP2;
					break;
				case LICH_ATK_RISING_RAIN:
					pose = MONSTER_POSE_SPECIAL_WINDUP1;
					break;
				case LICH_ATK_BASICSPELL_SINGLE:
					pose = MONSTER_POSE_MAGIC_WINDUP1;
					break;
				case LICH_ATK_RISING_SINGLE:
					pose = MONSTER_POSE_MELEE_WINDUP1;
					break;
				case LICH_ATK_VERTICAL_QUICK:
					pose = MONSTER_POSE_MELEE_WINDUP1;
					break;
				case LICH_ATK_HORIZONTAL_RETURN:
					pose = MONSTER_POSE_MELEE_WINDUP2;
					break;
				case LICH_ATK_HORIZONTAL_QUICK:
					pose = MONSTER_POSE_MELEE_WINDUP2;
					break;
				case LICH_ATK_CHARGE_AOE:
					pose = MONSTER_POSE_SPECIAL_WINDUP2;
					break;
				case LICH_ATK_FALLING_DIAGONAL:
					pose = MONSTER_POSE_SPECIAL_WINDUP3;
					break;
				case LICH_ATK_SUMMON:
					pose = MONSTER_POSE_MAGIC_WINDUP3;
					break;
				default:
					break;
			}
		}
		else if ( itemCategory(myStats->weapon) == MAGICSTAFF )
		{
			if ( myStats->type == KOBOLD || myStats->type == AUTOMATON 
				|| myStats->type == GOATMAN || myStats->type == INSECTOID 
				|| myStats->type == INCUBUS || myStats->type == VAMPIRE
				|| myStats->type == HUMAN || myStats->type == GOBLIN
				|| myStats->type == SKELETON || myStats->type == GNOME
				|| myStats->type == SUCCUBUS || myStats->type == SHOPKEEPER
				|| myStats->type == SHADOW )
			{
				pose = MONSTER_POSE_MELEE_WINDUP1;
			}
			else
			{
				pose = 3;  // jab
			}
		}
		else if ( itemCategory(myStats->weapon) == SPELLBOOK )
		{
			if ( myStats->type == INSECTOID && this->monsterSpecialTimer == MONSTER_SPECIAL_COOLDOWN_INSECTOID_ACID )
			{
				pose = MONSTER_POSE_MAGIC_WINDUP3;
			}
			else if ( myStats->type == INCUBUS && this->monsterSpecialTimer == MONSTER_SPECIAL_COOLDOWN_INCUBUS_STEAL )
			{
				pose = MONSTER_POSE_MAGIC_WINDUP3;
			}
			else if ( myStats->type == COCKATRICE && this->monsterSpecialTimer == MONSTER_SPECIAL_COOLDOWN_COCKATRICE_STONE )
			{
				pose = MONSTER_POSE_MAGIC_WINDUP2;
			}
			else if ( myStats->type == VAMPIRE )
			{
				if ( this->monsterSpecialTimer == MONSTER_SPECIAL_COOLDOWN_VAMPIRE_DRAIN )
				{
					pose = MONSTER_POSE_VAMPIRE_DRAIN;
				}
				else if ( this->monsterSpecialTimer == MONSTER_SPECIAL_COOLDOWN_VAMPIRE_AURA )
				{
					pose = MONSTER_POSE_VAMPIRE_AURA_CHARGE;
				}
				else
				{
					pose = MONSTER_POSE_MAGIC_WINDUP1;
				}
			}
			else if ( myStats->type == KOBOLD || myStats->type == AUTOMATON 
				|| myStats->type == GOATMAN || myStats->type == INSECTOID 
				|| myStats->type == COCKATRICE || myStats->type == INCUBUS 
				|| myStats->type == VAMPIRE || myStats->type == HUMAN
				|| myStats->type == GOBLIN || myStats->type == SKELETON 
				|| myStats->type == GNOME || myStats->type == SUCCUBUS
				|| myStats->type == SHOPKEEPER || myStats->type == SHADOW )
			{
				pose = MONSTER_POSE_MAGIC_WINDUP1;
			}
			else if ( myStats->type == DEMON || myStats->type == CREATURE_IMP )
			{
				pose = MONSTER_POSE_MELEE_WINDUP1;
			}
			else
			{
				pose = 1;  // vertical swing
			}
		}
		else if ( itemCategory(myStats->weapon) == POTION )
		{
			if ( myStats->type == GOATMAN )
			{
				/*if ( this->monsterSpecialTimer == MONSTER_SPECIAL_COOLDOWN_GOATMAN_DRINK )
				{
					pose = MONSTER_POSE_RANGED_WINDUP3;
				}
				else if ( this->monsterSpecialTimer == MONSTER_SPECIAL_COOLDOWN_GOATMAN_THROW )
				{
					pose = MONSTER_POSE_MELEE_WINDUP1;
				}*/
				if ( monsterSpecialState == GOATMAN_POTION )
				{
					pose = MONSTER_POSE_RANGED_WINDUP3;
				}

			}
			else if ( myStats->type == INCUBUS )
			{
				if ( this->monsterSpecialTimer == MONSTER_SPECIAL_COOLDOWN_INCUBUS_CONFUSION )
				{
					pose = MONSTER_POSE_SPECIAL_WINDUP1;
				}
			}
			else
			{
				pose = MONSTER_POSE_MELEE_WINDUP1;
			}
		}
		else if ( this->hasRangedWeapon() )
		{
			if ( myStats->type == KOBOLD || myStats->type == AUTOMATON 
				|| myStats->type == GOATMAN || myStats->type == INSECTOID 
				|| myStats->type == INCUBUS || myStats->type == VAMPIRE
				|| myStats->type == HUMAN || myStats->type == GOBLIN 
				|| myStats->type == SKELETON || myStats->type == GNOME
				|| myStats->type == SUCCUBUS || myStats->type == SHOPKEEPER
				|| myStats->type == SHADOW )
			{
				if ( myStats->weapon->type == CROSSBOW )
				{
					pose = MONSTER_POSE_RANGED_WINDUP1;
				}
				else if ( itemCategory(myStats->weapon) == THROWN )
				{
					if ( myStats->type == INSECTOID )
					{
						if ( this->monsterSpecialTimer == MONSTER_SPECIAL_COOLDOWN_INSECTOID_THROW )
						{
							pose = MONSTER_POSE_RANGED_WINDUP3;
						}
						else
						{
							pose = MONSTER_POSE_MELEE_WINDUP1;
						}
					}
					else
					{
						pose = MONSTER_POSE_MELEE_WINDUP1;
					}
				}
				else
				{
					pose = MONSTER_POSE_RANGED_WINDUP2;
				}
			}
			else
			{
				pose = 0;
			}
		}
		else
		{
			if ( myStats->type == KOBOLD || myStats->type == AUTOMATON 
				|| myStats->type == GOATMAN || myStats->type == INSECTOID 
				|| myStats->type == INCUBUS || myStats->type == VAMPIRE
				|| myStats->type == HUMAN || myStats->type == GOBLIN
				|| myStats->type == SKELETON || myStats->type == GNOME
				|| myStats->type == SUCCUBUS || myStats->type == SHOPKEEPER
				|| myStats->type == SHADOW )
			{
				if ( getWeaponSkill(myStats->weapon) == PRO_AXE || getWeaponSkill(myStats->weapon) == PRO_MACE )
				{
					// axes and maces don't stab
					pose = MONSTER_POSE_MELEE_WINDUP1 + rand() % 2;
				}
				else
				{
					pose = MONSTER_POSE_MELEE_WINDUP1 + rand() % 3;
				}
			}
			else
			{
				pose = rand() % 3 + 1;
			}
		}
	}
	// fists
	else
	{
		if ( myStats->type == KOBOLD || myStats->type == AUTOMATON 
			|| myStats->type == GOATMAN || myStats->type == INSECTOID 
			|| myStats->type == INCUBUS || myStats->type == VAMPIRE
			|| myStats->type == HUMAN || myStats->type == GOBLIN
			|| myStats->type == GHOUL || myStats->type == SKELETON
			|| myStats->type == GNOME || myStats->type == DEMON
			|| myStats->type == CREATURE_IMP || myStats->type == SUCCUBUS
			|| myStats->type == SHOPKEEPER || myStats->type == MINOTAUR
			|| myStats->type == SHADOW )
		{
			pose = MONSTER_POSE_MELEE_WINDUP1;
		}
		else if ( myStats->type == CRYSTALGOLEM )
		{
			if ( this->monsterSpecialTimer == MONSTER_SPECIAL_COOLDOWN_GOLEM )
			{
				pose = MONSTER_POSE_MELEE_WINDUP3;
			}
			else
			{
				pose = MONSTER_POSE_MELEE_WINDUP1 + rand() % 2;
			}
		}
		else if ( myStats->type == COCKATRICE )
		{
			if ( this->monsterSpecialTimer == MONSTER_SPECIAL_COOLDOWN_COCKATRICE_ATK )
			{
				pose = MONSTER_POSE_MELEE_WINDUP3;
			}
			else
			{
				pose = MONSTER_POSE_MELEE_WINDUP1 + rand() % 2;
			}
		}
		else if ( myStats->type == TROLL )
		{
			pose = MONSTER_POSE_MELEE_WINDUP1;
		}
		else
		{
			pose = 1;
		}
	}

	return pose;
}

bool Entity::hasRangedWeapon() const
{
	Stat *myStats = getStats();
	if ( myStats == nullptr || myStats->weapon == nullptr )
	{
		return false;
	}

	if ( myStats->weapon->type == SLING )
	{
		return true;
	}
	else if ( myStats->weapon->type == SHORTBOW )
	{
		return true;
	}
	else if ( myStats->weapon->type == CROSSBOW )
	{
		return true;
	}
	else if ( myStats->weapon->type == ARTIFACT_BOW )
	{
		return true;
	}
	else if ( itemCategory(myStats->weapon) == MAGICSTAFF )
	{
		return true;
	}
	else if ( itemCategory(myStats->weapon) == SPELLBOOK )
	{
		return true;
	}
	else if ( itemCategory(myStats->weapon) == THROWN )
	{
		return true;
	}
	else if ( itemCategory(myStats->weapon) == GEM )
	{
		return true;
	}
	else if ( itemCategory(myStats->weapon) == POTION )
	{
		return true;
	}

	return false;
}

/*void Entity::returnWeaponarmToNeutral(Entity* weaponarm, Entity* rightbody)
{
weaponarm->skill[0] = rightbody->skill[0];
monsterWeaponYaw = 0;
weaponarm->pitch = rightbody->pitch;
weaponarm->roll = 0;
monsterArmbended = 0;
monsterAttack = 0;
}*/

void Entity::handleWeaponArmAttack(Entity* weaponarm)
{
	if ( weaponarm == nullptr )
	{
		return;
	}

	Entity* rightbody = nullptr;
	// set rightbody to left leg.
	node_t* rightbodyNode = list_Node(&this->children, LIMB_HUMANOID_LEFTLEG);
	if ( rightbodyNode )
	{
		rightbody = (Entity*)rightbodyNode->element;
	}
	else
	{
		return;
	}

	// vertical chop windup
	if ( monsterAttack == MONSTER_POSE_MELEE_WINDUP1 )
	{
		if ( monsterAttackTime == 0 )
		{
			// init rotations
			weaponarm->pitch = 0;
			this->monsterArmbended = 0;
			this->monsterWeaponYaw = 0;
			weaponarm->roll = 0;
			weaponarm->skill[1] = 0;
		}

		limbAnimateToLimit(weaponarm, ANIMATE_PITCH, -0.25, 5 * PI / 4, false, 0.0);

		if ( monsterAttackTime >= ANIMATE_DURATION_WINDUP / (monsterGlobalAnimationMultiplier / 10.0) )
		{
			if ( multiplayer != CLIENT )
			{
				this->attack(1, 0, nullptr);
			}
		}
	}
	// vertical chop attack
	else if ( monsterAttack == 1 )
	{
		if ( weaponarm->pitch >= 3 * PI / 2 )
		{
			this->monsterArmbended = 1;
		}

		if ( weaponarm->skill[1] == 0 )
		{
			// chop forwards
			if ( limbAnimateToLimit(weaponarm, ANIMATE_PITCH, 0.4, PI / 3, false, 0.0) )
			{
				weaponarm->skill[1] = 1;
			}
		}
		else if ( weaponarm->skill[1] >= 1 )
		{
			if ( limbAnimateToLimit(weaponarm, ANIMATE_PITCH, -0.25, 7 * PI / 4, false, 0.0) )
			{
				weaponarm->skill[0] = rightbody->skill[0];
				this->monsterWeaponYaw = 0;
				weaponarm->pitch = rightbody->pitch;
				weaponarm->roll = 0;
				this->monsterArmbended = 0;
				monsterAttack = 0;
				//returnWeaponarmToNeutral(weaponarm, rightbody);
			}
		}
	}
	// horizontal chop windup
	else if ( monsterAttack == MONSTER_POSE_MELEE_WINDUP2 )
	{
		if ( monsterAttackTime == 0 )
		{
			// init rotations
			weaponarm->pitch = PI / 4;
			weaponarm->roll = 0;
			this->monsterArmbended = 1;
			weaponarm->skill[1] = 0;
			this->monsterWeaponYaw = 6 * PI / 4;
		}

		limbAnimateToLimit(weaponarm, ANIMATE_ROLL, -0.2, 3 * PI / 2, false, 0.0);
		limbAnimateToLimit(weaponarm, ANIMATE_PITCH, -0.2, 0, false, 0.0);


		if ( monsterAttackTime >= ANIMATE_DURATION_WINDUP / (monsterGlobalAnimationMultiplier / 10.0) )
		{
			if ( multiplayer != CLIENT )
			{
				this->attack(2, 0, nullptr);
			}
		}
	}
	// horizontal chop attack
	else if ( monsterAttack == 2 )
	{
		if ( weaponarm->skill[1] == 0 )
		{
			// swing
			// this->weaponyaw is OK to change for clients, as server doesn't update it for them.
			if ( limbAnimateToLimit(this, ANIMATE_WEAPON_YAW, 0.3, 2 * PI / 8, false, 0.0) )
			{
				weaponarm->skill[1] = 1;
			}
		}
		else if ( weaponarm->skill[1] >= 1 )
		{
			// post-swing return to normal weapon yaw
			if ( limbAnimateToLimit(this, ANIMATE_WEAPON_YAW, -0.5, 0, false, 0.0) )
			{
				// restore pitch and roll after yaw is set
				if ( limbAnimateToLimit(weaponarm, ANIMATE_ROLL, 0.4, 0, false, 0.0)
					&& limbAnimateToLimit(weaponarm, ANIMATE_PITCH, -0.4, 7 * PI / 4, false, 0.0) )
				{
					weaponarm->skill[0] = rightbody->skill[0];
					this->monsterWeaponYaw = 0;
					weaponarm->pitch = rightbody->pitch;
					weaponarm->roll = 0;
					this->monsterArmbended = 0;
					monsterAttack = 0;
				}
			}
		}
	}
	// stab windup
	else if ( monsterAttack == MONSTER_POSE_MELEE_WINDUP3 )
	{
		if ( monsterAttackTime == 0 )
		{
			// init rotations
			this->monsterArmbended = 0;
			this->monsterWeaponYaw = 0;
			weaponarm->roll = 0;
			weaponarm->pitch = 0;
			weaponarm->skill[1] = 0;
		}

		limbAnimateToLimit(weaponarm, ANIMATE_PITCH, 0.5, 2 * PI / 3, false, 0.0);

		if ( monsterAttackTime >= ANIMATE_DURATION_WINDUP / (monsterGlobalAnimationMultiplier / 10.0) )
		{
			if ( multiplayer != CLIENT )
			{
				this->attack(3, 0, nullptr);
			}
		}
	}
	// stab attack - refer to weapon limb code for additional animation
	else if ( monsterAttack == 3 )
	{
		if ( weaponarm->skill[1] == 0 )
		{
			if ( limbAnimateToLimit(weaponarm, ANIMATE_PITCH, -0.3, 0, false, 0.0) )
			{
				weaponarm->skill[1] = 1;
			}
		}
		else if ( weaponarm->skill[1] == 1 )
		{
			if ( limbAnimateToLimit(weaponarm, ANIMATE_PITCH, 0.3, 2 * PI / 3, false, 0.0) )
			{
				weaponarm->skill[1] = 2;
			}
		}
		else if ( weaponarm->skill[1] >= 2 )
		{
			// return to neutral
			if ( limbAnimateToLimit(weaponarm, ANIMATE_PITCH, -0.2, 0, false, 0.0) )
			{
				weaponarm->skill[0] = rightbody->skill[0];
				this->monsterWeaponYaw = 0;
				weaponarm->pitch = rightbody->pitch;
				weaponarm->roll = 0;
				this->monsterArmbended = 0;
				monsterAttack = 0;
			}
		}
	}
	// ranged weapons
	else if ( monsterAttack == MONSTER_POSE_RANGED_WINDUP1 )
	{
		// crossbow
		if ( monsterAttackTime == 0 )
		{
			// init rotations
			this->monsterArmbended = 0;
			this->monsterWeaponYaw = 0;
			weaponarm->roll = 0;
			weaponarm->skill[1] = 0;
		}

		// draw the crossbow level... slowly
		if ( weaponarm->pitch > PI || weaponarm->pitch < 0 )
		{
			limbAnimateToLimit(weaponarm, ANIMATE_PITCH, 0.1, 0, false, 0.0);
		}
		else
		{
			limbAnimateToLimit(weaponarm, ANIMATE_PITCH, -0.1, 0, false, 0.0);
		}

		if ( monsterAttackTime >= ANIMATE_DURATION_WINDUP / (monsterGlobalAnimationMultiplier / 10.0) )
		{
			if ( multiplayer != CLIENT )
			{
				this->attack(MONSTER_POSE_RANGED_SHOOT1, 0, nullptr);
			}
		}
	}
	// shoot crossbow
	else if ( monsterAttack == MONSTER_POSE_RANGED_SHOOT1 )
	{
		// recoil upwards
		if ( weaponarm->skill[1] == 0 )
		{
			if ( limbAnimateToLimit(weaponarm, ANIMATE_PITCH, -0.2, 15 * PI / 8, false, 0.0) )
			{
				weaponarm->skill[1] = 1;
			}
		}
		// recoil downwards
		else if ( weaponarm->skill[1] == 1 )
		{
			if ( limbAnimateToLimit(weaponarm, ANIMATE_PITCH, 0.1, PI / 3, false, 0.0) )
			{
				weaponarm->skill[1] = 2;
			}
		}
		else if ( weaponarm->skill[1] >= 2 )
		{
			// limbAngleWithinRange cuts off animation early so it doesn't snap too far back to position.
			if ( limbAnimateToLimit(weaponarm, ANIMATE_PITCH, -0.2, 0, false, 0.0) || limbAngleWithinRange(weaponarm->pitch, -0.2, rightbody->pitch) )
			{
				weaponarm->skill[0] = rightbody->skill[0];
				this->monsterWeaponYaw = 0;
				//if ( this->hasRangedWeapon() && this->monsterState == MONSTER_STATE_ATTACK )
				//{
				//	// don't move ranged weapons so far if ready to attack
				//	weaponarm->pitch = rightbody->pitch * 0.25;
				//}
				//else
				//{
				//	weaponarm->pitch = rightbody->pitch;
				//}
				weaponarm->roll = 0;
				this->monsterArmbended = 0;
				monsterAttack = 0;
			}
		}
	}
	// shortbow/sling
	else if ( monsterAttack == MONSTER_POSE_RANGED_WINDUP2 )
	{
		if ( monsterAttackTime == 0 )
		{
			// init rotations
			this->monsterArmbended = 0;
			this->monsterWeaponYaw = 0;
			weaponarm->roll = 0;
			weaponarm->skill[1] = 0;
		}

		// draw the weapon level... slowly and shake
		if ( weaponarm->pitch > PI || weaponarm->pitch < 0 )
		{
			limbAnimateToLimit(weaponarm, ANIMATE_PITCH, 0.1, 0, true, 0.1);
		}
		else
		{
			limbAnimateToLimit(weaponarm, ANIMATE_PITCH, -0.1, 0, true, 0.1);
		}

		if ( monsterAttackTime >= ANIMATE_DURATION_WINDUP / (monsterGlobalAnimationMultiplier / 10.0) )
		{
			if ( multiplayer != CLIENT )
			{
				this->attack(MONSTER_POSE_RANGED_SHOOT2, 0, nullptr);
			}
		}
	}
	// shoot shortbow/sling
	else if ( monsterAttack == MONSTER_POSE_RANGED_SHOOT2 )
	{
		// recoil upwards
		if ( weaponarm->skill[1] == 0 )
		{
			if ( limbAnimateToLimit(weaponarm, ANIMATE_PITCH, -0.2, 14 * PI / 8, false, 0.0) )
			{
				weaponarm->skill[1] = 1;
			}
		}
		// recoil downwards
		else if ( weaponarm->skill[1] == 1 )
		{
			if ( limbAnimateToLimit(weaponarm, ANIMATE_PITCH, 0.1, 1 * PI / 3, false, 0.0) )
			{
				weaponarm->skill[1] = 2;
			}
		}
		else if ( weaponarm->skill[1] >= 2 )
		{
			// limbAngleWithinRange cuts off animation early so it doesn't snap too far back to position.
			if ( limbAnimateToLimit(weaponarm, ANIMATE_PITCH, -0.2, 0, false, 0.0) || limbAngleWithinRange(weaponarm->pitch, -0.2, rightbody->pitch) )
			{
				weaponarm->skill[0] = rightbody->skill[0];
				this->monsterWeaponYaw = 0;
				weaponarm->pitch = rightbody->pitch;
				weaponarm->roll = 0;
				this->monsterArmbended = 0;
				monsterAttack = 0;
				// play draw arrow sound
				playSoundEntityLocal(this, 246, 16);
			}
		}
	}
	else if ( monsterAttack == MONSTER_POSE_MAGIC_WINDUP1 )
	{
		// magic wiggle hands
		if ( monsterAttackTime == 0 )
		{
			// init rotations
			this->monsterArmbended = 0;
			this->monsterWeaponYaw = 0;
			weaponarm->roll = 0;
			weaponarm->pitch = 0;
			weaponarm->yaw = this->yaw;
			weaponarm->skill[1] = 0;
			// casting particles
			createParticleDot(this);
			// play casting sound
			playSoundEntityLocal(this, 170, 32);
		}

		double animationYawSetpoint = 0.f;
		double animationYawEndpoint = 0.f;
		double armSwingRate = 0.f;
		double animationPitchSetpoint = 0.f;
		double animationPitchEndpoint = 0.f;

		switch ( this->monsterSpellAnimation )
		{
			case MONSTER_SPELLCAST_NONE:
				break;
			case MONSTER_SPELLCAST_SMALL_HUMANOID:
				// smaller models so arms can wave in a larger radius and faster.
				animationYawSetpoint = normaliseAngle2PI(this->yaw + 2 * PI / 8);
				animationYawEndpoint = normaliseAngle2PI(this->yaw - 2 * PI / 8);
				animationPitchSetpoint = 2 * PI / 8;
				animationPitchEndpoint = 14 * PI / 8;
				armSwingRate = 0.3;
				if ( monsterAttackTime == 0 )
				{
					weaponarm->yaw = this->yaw - PI / 8;
				}
				break;
			case MONSTER_SPELLCAST_HUMANOID:
				animationYawSetpoint = normaliseAngle2PI(this->yaw + 1 * PI / 8);
				animationYawEndpoint = normaliseAngle2PI(this->yaw - 1 * PI / 8);
				animationPitchSetpoint = 1 * PI / 8;
				animationPitchEndpoint = 15 * PI / 8;
				armSwingRate = 0.15;
				break;
			default:
				break;
		}

		if ( weaponarm->skill[1] == 0 )
		{
			if ( limbAnimateToLimit(weaponarm, ANIMATE_PITCH, armSwingRate, animationPitchSetpoint, false, 0.0) )
			{
				if ( limbAnimateToLimit(weaponarm, ANIMATE_YAW, armSwingRate, animationYawSetpoint, false, 0.0) )
				{
					weaponarm->skill[1] = 1;
				}
			}
		}
		else
		{
			if ( limbAnimateToLimit(weaponarm, ANIMATE_PITCH, -armSwingRate, animationPitchEndpoint, false, 0.0) )
			{
				if ( limbAnimateToLimit(weaponarm, ANIMATE_YAW, -armSwingRate, animationYawEndpoint, false, 0.0) )
				{
					weaponarm->skill[1] = 0;
				}
			}
		}

		if ( monsterAttackTime >= 2 * ANIMATE_DURATION_WINDUP / (monsterGlobalAnimationMultiplier / 10.0) )
		{
			if ( multiplayer != CLIENT )
			{
				// swing the arm after we prepped the spell
				this->attack(MONSTER_POSE_MAGIC_WINDUP2, 0, nullptr);
			}
		}
	}
	// swing arm to cast spell
	else if ( monsterAttack == MONSTER_POSE_MAGIC_WINDUP2 )
	{
		if ( monsterAttackTime == 0 )
		{
			// init rotations
			weaponarm->pitch = 0;
			this->monsterArmbended = 0;
			this->monsterWeaponYaw = 0;
			weaponarm->roll = 0;
			weaponarm->skill[1] = 0;
		}

		if ( limbAnimateToLimit(weaponarm, ANIMATE_PITCH, -0.3, 5 * PI / 4, false, 0.0) )
		{
			if ( multiplayer != CLIENT )
			{
				Stat* stats = this->getStats();
				if ( stats && stats->type == SHADOW )
				{
					this->attack(MONSTER_POSE_MAGIC_CAST1, 0, nullptr);
				}
				else
				{
					this->attack(1, 0, nullptr);
				}
			}
		}
	}

	return;
}

void Entity::humanoidAnimateWalk(Entity* limb, node_t* bodypartNode, int bodypart, double walkSpeed, double dist, double distForFootstepSound)
{
	if ( bodypart == LIMB_HUMANOID_RIGHTLEG || bodypart == LIMB_HUMANOID_LEFTARM )
	{
		Entity* rightbody = nullptr;
		// set rightbody to left leg.
		node_t* rightbodyNode = list_Node(&this->children, LIMB_HUMANOID_LEFTLEG);
		if ( rightbodyNode )
		{
			rightbody = (Entity*)rightbodyNode->element;
		}
		else
		{
			return;
		}

		node_t* shieldNode = list_Node(&this->children, 8);
		if ( shieldNode )
		{
			Entity* shield = (Entity*)shieldNode->element;
			if ( dist > 0.1 && (bodypart != LIMB_HUMANOID_LEFTARM || shield->sprite == 0) )
			{
				// walking to destination
				if ( !rightbody->skill[0] )
				{
					limb->pitch -= dist * walkSpeed;
					if ( limb->pitch < -PI / 4.0 )
					{
						limb->pitch = -PI / 4.0;
						if ( bodypart == LIMB_HUMANOID_RIGHTLEG )
						{
							if ( dist > distForFootstepSound )
							{
								if ( limb->skill[0] == 0 ) // fix for waking up on sleep to reduce repeated sound bytes in race condition.
								{
									if ( this->monsterFootstepType == MONSTER_FOOTSTEP_USE_BOOTS )
									{
										node_t* tempNode = list_Node(&this->children, 3);
										if ( tempNode )
										{
											Entity* foot = (Entity*)tempNode->element;
											playSoundEntityLocal(this, getMonsterFootstepSound(this->monsterFootstepType, foot->sprite), 32);
										}
									}
									else
									{
										playSoundEntityLocal(this, getMonsterFootstepSound(this->monsterFootstepType, 0), 32);
									}
									limb->skill[0] = 1;
								}
							}
						}
					}
				}
				else
				{
					limb->pitch += dist * walkSpeed;
					if ( limb->pitch > PI / 4.0 )
					{
						limb->pitch = PI / 4.0;
						if ( bodypart == LIMB_HUMANOID_RIGHTLEG )
						{
							if ( dist > distForFootstepSound )
							{
								if ( limb->skill[0] == 1 ) // fix for waking up on sleep to reduce repeated sound bytes in race condition.
								{
									if ( this->monsterFootstepType == MONSTER_FOOTSTEP_USE_BOOTS )
									{
										node_t* tempNode = list_Node(&this->children, 3);
										if ( tempNode )
										{
											Entity* foot = (Entity*)tempNode->element;
											playSoundEntityLocal(this, getMonsterFootstepSound(this->monsterFootstepType, foot->sprite), 32);
										}
									}
									else
									{
										playSoundEntityLocal(this, getMonsterFootstepSound(this->monsterFootstepType, 0), 32);
									}
									limb->skill[0] = 0;
								}
							}
						}
					}
				}
			}
			else
			{
				// coming to a stop
				if ( limb->pitch < 0 || (limb->pitch > PI && limb->pitch < 2 * PI) )
				{
					limb->pitch += 1 / fmax(dist * .1, 10.0);
					if ( limb->pitch > 0 )
					{
						limb->pitch = 0;
					}
				}
				else if ( limb->pitch > 0 )
				{
					limb->pitch -= 1 / fmax(dist * .1, 10.0);
					if ( limb->pitch < 0 )
					{
						limb->pitch = 0;
					}
				}
			}
		}
	}
	else if ( bodypart == LIMB_HUMANOID_LEFTLEG || bodypart == LIMB_HUMANOID_RIGHTARM || bodypart == LIMB_HUMANOID_CLOAK )
	{
		if ( bodypart != LIMB_HUMANOID_RIGHTARM || (this->monsterAttack == 0 && this->monsterAttackTime == 0) )
		{
			if ( dist > 0.1 )
			{
				double armMoveSpeed = 1.0;
				if ( bodypart == LIMB_HUMANOID_RIGHTARM && this->hasRangedWeapon() && this->monsterState == MONSTER_STATE_ATTACK )
				{
					// don't move ranged weapons so far if ready to attack
					armMoveSpeed = 0.5;
				}

				if ( limb->skill[0] )
				{
					limb->pitch -= dist * walkSpeed * armMoveSpeed;
					if ( limb->pitch < -PI * armMoveSpeed / 4.0 )
					{
						limb->skill[0] = 0;
						limb->pitch = -PI * armMoveSpeed / 4.0;
					}
				}
				else
				{
					limb->pitch += dist * walkSpeed * armMoveSpeed;
					if ( limb->pitch > PI * armMoveSpeed / 4.0 )
					{
						limb->skill[0] = 1;
						limb->pitch = PI * armMoveSpeed / 4.0;
					}
				}
			}
			else
			{
				if ( limb->pitch < 0 )
				{
					limb->pitch += 1 / fmax(dist * .1, 10.0);
					if ( limb->pitch > 0 )
					{
						limb->pitch = 0;
					}
				}
				else if ( limb->pitch > 0 )
				{
					limb->pitch -= 1 / fmax(dist * .1, 10.0);
					if ( limb->pitch < 0 )
					{
						limb->pitch = 0;
					}
				}
			}
		}
	}

	return;
}

Uint32 Entity::getMonsterFootstepSound(int footstepType, int bootSprite)
{
	int sound = -1;

	switch ( footstepType )
	{
		case MONSTER_FOOTSTEP_SKELETON:
			sound = 95;
			break;
		case MONSTER_FOOTSTEP_STOMP:
			sound = 115;
			break;
		case MONSTER_FOOTSTEP_LEATHER:
			sound = rand() % 7;
			break;
		case MONSTER_FOOTSTEP_USE_BOOTS:
			if ( bootSprite >= 152 && bootSprite <= 155 ) // iron boots
			{
				sound = 7 + rand() % 7;
			}
			else if ( bootSprite >= 156 && bootSprite <= 159 ) // steel boots
			{
				sound = 14 + rand() % 7;
			}
			else if ( bootSprite >= 499 && bootSprite <= 502 ) // crystal boots
			{
				sound = 14 + rand() % 7;
			}
			else if ( bootSprite >= 521 && bootSprite <= 524 ) // artifact boots
			{
				sound = 14 + rand() % 7;
			}
			else
			{
				sound = rand() % 7;
			}
			break;
		case MONSTER_FOOTSTEP_NONE:
		default:
			break;
	}
	return static_cast<Uint32>(sound);
}

void Entity::handleHumanoidWeaponLimb(Entity* weaponLimb, Entity* weaponArmLimb)
{
	if ( weaponLimb == nullptr || weaponArmLimb == nullptr )
	{
		return;
	}

	int monsterType = this->getMonsterTypeFromSprite();
	int myAttack = this->monsterAttack;
	bool isPlayer = this->behavior == &actPlayer;
	if ( isPlayer )
	{
		myAttack = this->skill[9];
	}

	if ( weaponLimb->flags[INVISIBLE] == false ) //TODO: isInvisible()?
	{
		if ( weaponLimb->sprite == items[SHORTBOW].index )
		{
			weaponLimb->x = weaponArmLimb->x - .5 * cos(weaponArmLimb->yaw);
			weaponLimb->y = weaponArmLimb->y - .5 * sin(weaponArmLimb->yaw);
			weaponLimb->z = weaponArmLimb->z + 1;
			weaponLimb->pitch = weaponArmLimb->pitch + .25;
		}
		else if ( weaponLimb->sprite == items[ARTIFACT_BOW].index )
		{
			if ( isPlayer && monsterType == HUMAN )
			{
				weaponLimb->x = weaponArmLimb->x - .5 * cos(weaponArmLimb->yaw);
				weaponLimb->y = weaponArmLimb->y - .5 * sin(weaponArmLimb->yaw);
				weaponLimb->z = weaponArmLimb->z + 1;
				weaponLimb->pitch = weaponArmLimb->pitch + .25;
			}
			else
			{
				weaponLimb->x = weaponArmLimb->x - 1.5 * cos(weaponArmLimb->yaw);
				weaponLimb->y = weaponArmLimb->y - 1.5 * sin(weaponArmLimb->yaw);
				weaponLimb->z = weaponArmLimb->z + 2;
				weaponLimb->pitch = weaponArmLimb->pitch + .25;
			}
		}
		else if ( weaponLimb->sprite == items[CROSSBOW].index )
		{
			weaponLimb->x = weaponArmLimb->x;
			weaponLimb->y = weaponArmLimb->y;
			weaponLimb->z = weaponArmLimb->z + 1;
			weaponLimb->pitch = weaponArmLimb->pitch;
		}
		else if ( weaponLimb->sprite == items[TOOL_LOCKPICK].index )
		{
			weaponLimb->x = weaponArmLimb->x + 1.5 * cos(weaponArmLimb->yaw);
			weaponLimb->y = weaponArmLimb->y + 1.5 * sin(weaponArmLimb->yaw);
			weaponLimb->z = weaponArmLimb->z + 1.5;
			weaponLimb->pitch = weaponArmLimb->pitch + .25;
		}
		else
		{
			/*weaponLimb->focalx = limbs[monsterType][6][0];
			weaponLimb->focalz = limbs[monsterType][6][2];*/
			if ( myAttack == 3 && !isPlayer )
			{
				// poking animation, weapon pointing straight ahead.
				if ( weaponArmLimb->skill[1] < 2 && weaponArmLimb->pitch < PI / 2 )
				{
					// cos(weaponArmLimb->pitch)) * cos(weaponArmLimb->yaw) allows forward/back motion dependent on the arm rotation.
					weaponLimb->x = weaponArmLimb->x + (3 * cos(weaponArmLimb->pitch)) * cos(weaponArmLimb->yaw);
					weaponLimb->y = weaponArmLimb->y + (3 * cos(weaponArmLimb->pitch)) * sin(weaponArmLimb->yaw);

					if ( weaponArmLimb->pitch < PI / 3 )
					{
						// adjust the z point halfway through swing.
						weaponLimb->z = weaponArmLimb->z + 1.5 - 2 * cos(weaponArmLimb->pitch / 2);
						if ( monsterType == INCUBUS || monsterType == SUCCUBUS )
						{
							weaponLimb->z += 2;
						}
					}
					else
					{
						weaponLimb->z = weaponArmLimb->z - .5 * (myAttack == 0);
						if ( weaponLimb->pitch > PI / 2 )
						{
							limbAnimateToLimit(weaponLimb, ANIMATE_PITCH, -0.5, PI * 0.5, false, 0);
						}
						else
						{
							limbAnimateToLimit(weaponLimb, ANIMATE_PITCH, 0.5, PI * 0.5, false, 0);
						}
						if ( monsterType == INCUBUS || monsterType == SUCCUBUS )
						{
							weaponLimb->z += 1.25;
						}
					}
				}
				// hold sword with pitch aligned to arm rotation.
				else
				{
					weaponLimb->x = weaponArmLimb->x + .5 * cos(weaponArmLimb->yaw) * (myAttack == 0);
					weaponLimb->y = weaponArmLimb->y + .5 * sin(weaponArmLimb->yaw) * (myAttack == 0);
					weaponLimb->z = weaponArmLimb->z - .5;
					weaponLimb->pitch = weaponArmLimb->pitch + .25 * (myAttack == 0);
					if ( monsterType == INCUBUS || monsterType == SUCCUBUS )
					{
						weaponLimb->z += 1;
					}
				}
			}
			else
			{
				weaponLimb->x = weaponArmLimb->x + .5 * cos(weaponArmLimb->yaw) * (myAttack == 0);
				weaponLimb->y = weaponArmLimb->y + .5 * sin(weaponArmLimb->yaw) * (myAttack == 0);
				weaponLimb->z = weaponArmLimb->z - .5 * (myAttack == 0);
				weaponLimb->pitch = weaponArmLimb->pitch + .25 * (myAttack == 0);
			}
		}
	}

	weaponLimb->yaw = weaponArmLimb->yaw;

	if ( myAttack == MONSTER_POSE_RANGED_WINDUP3 && monsterType == GOATMAN )
	{
		// specific for potion throwing goatmen.
		limbAnimateToLimit(weaponLimb, ANIMATE_ROLL, 0.25, 1 * PI / 4, false, 0.0);
	}
	else
	{
		weaponLimb->roll = weaponArmLimb->roll;
		if ( isPlayer )
		{
			if ( weaponLimb->sprite >= 50 && weaponLimb->sprite < 58 )
			{
				weaponLimb->roll += (PI / 2); // potion sprites rotated
			}
		}
	}

	bool armBended = (!isPlayer && this->monsterArmbended) || (isPlayer && this->skill[11]);

	if ( !armBended )
	{
		weaponLimb->focalx = limbs[monsterType][6][0]; // 2.5
		if ( weaponLimb->sprite == items[CROSSBOW].index )
		{
			weaponLimb->focalx += 2;
		}
		weaponLimb->focaly = limbs[monsterType][6][1]; // 0
		weaponLimb->focalz = limbs[monsterType][6][2]; // -.5
	}
	else
	{
		if ( monsterType == INCUBUS || monsterType == SUCCUBUS )
		{
			weaponLimb->focalx = limbs[monsterType][6][0] + 2; // 3.5
			weaponLimb->focaly = limbs[monsterType][6][1]; // 0
			weaponLimb->focalz = limbs[monsterType][6][2] - 3.5; // -2.5
		}
		else if ( isPlayer && monsterType == HUMAN )
		{
			weaponLimb->focalx = limbs[monsterType][6][0] + 1.5;
			weaponLimb->focaly = limbs[monsterType][6][1];
			weaponLimb->focalz = limbs[monsterType][6][2] - 2;
		}
		else
		{
			weaponLimb->focalx = limbs[monsterType][6][0] + 1; // 3.5
			weaponLimb->focaly = limbs[monsterType][6][1]; // 0
			weaponLimb->focalz = limbs[monsterType][6][2] - 2; // -2.5
		}
		weaponLimb->yaw -= sin(weaponArmLimb->roll) * PI / 2;
		weaponLimb->pitch += cos(weaponArmLimb->roll) * PI / 2;
	}

	return;
}

void Entity::lookAtEntity(Entity& target)
{
	double tangent = atan2(target.y - y, target.x - x);
	monsterLookTime = 1;
	monsterMoveTime = rand() % 10 + 1;
	monsterLookDir = tangent;
}

spell_t* Entity::getActiveMagicEffect(int spellID)
{
	Stat* myStats = getStats();
	if ( !myStats )
	{
		return nullptr;
	}

	spell_t* spell = nullptr;
	spell_t* searchSpell = nullptr;

	for ( node_t *node = myStats->magic_effects.first; node; node = node->next )
	{
		searchSpell = (node->element ? static_cast<spell_t*>(node->element) : nullptr);
		if ( searchSpell && searchSpell->ID == spellID )
		{
			spell = searchSpell;
			break;
		}
	}

	return spell;
}

void actAmbientParticleEffectIdle(Entity* my)
{
	if ( !my )
	{
		return;
	}

	if ( my->particleDuration < 0 )
	{
		list_RemoveNode(my->mynode);
		return;
	}
	else
	{
		if ( my->particleShrink == 1 )
		{
			// shrink the particle.
			my->scalex *= 0.95;
			my->scaley *= 0.95;
			my->scalez *= 0.95;
		}
		--my->particleDuration;
		my->z += my->vel_z;
		my->yaw += 0.1;
		if ( my->yaw > 2 * PI )
		{
			my->yaw = 0;
		}
	}

	return;
}

void Entity::spawnAmbientParticles(int chance, int particleSprite, int duration, double particleScale, bool shrink)
{
	if ( rand() % chance == 0 )
	{
		Entity* spawnParticle = newEntity(particleSprite, 1, map.entities, nullptr); //Particle entity.
		spawnParticle->sizex = 1;
		spawnParticle->sizey = 1;
		spawnParticle->x = x + (-2 + rand() % 5);
		spawnParticle->y = y + (-2 + rand() % 5);
		spawnParticle->z = 7.5;
		spawnParticle->scalex *= particleScale;
		spawnParticle->scaley *= particleScale;
		spawnParticle->scalez *= particleScale;
		spawnParticle->vel_z = -1;
		spawnParticle->particleDuration = duration;
		if ( shrink )
		{
			spawnParticle->particleShrink = 1;
		}
		else
		{
			spawnParticle->particleShrink = 0;
		}
		spawnParticle->behavior = &actAmbientParticleEffectIdle;
spawnParticle->flags[PASSABLE] = true;
spawnParticle->setUID(-3);
	}
}

void Entity::handleEffectsClient()
{
	Stat* myStats = getStats();

	if ( !myStats )
	{
		return;
	}

	if ( myStats->EFFECTS[EFF_MAGICREFLECT] )
	{
		spawnAmbientParticles(80, 579, 10 + rand() % 40, 1.0, false);
	}

	if ( myStats->EFFECTS[EFF_VAMPIRICAURA] )
	{
		spawnAmbientParticles(30, 600, 20 + rand() % 30, 0.5, true);
	}

	if ( myStats->EFFECTS[EFF_PACIFY] )
	{
		spawnAmbientParticles(30, 685, 20 + rand() % 30, 0.5, true);
	}

	if ( myStats->EFFECTS[EFF_POLYMORPH] )
	{
		if ( ticks % 25 == 0 || ticks % 40 == 0 )
		{
			spawnAmbientParticles(1, 593, 20 + rand() % 10, 0.5, true);
		}
	}

	if ( myStats->EFFECTS[EFF_INVISIBLE] && getMonsterTypeFromSprite() == SHADOW )
	{
		spawnAmbientParticles(20, 175, 20 + rand() % 30, 0.5, true);
	}
}

void Entity::serverUpdateEffectsForEntity(bool guarantee)
{
	if ( multiplayer != SERVER )
	{
		return;
	}

	Stat* myStats = getStats();

	if ( !myStats )
	{
		return;
	}

	for ( int player = 1; player < numplayers; ++player )
	{
		if ( client_disconnected[player] )
		{
			continue;
		}

		/*
		* Packet breakdown:
		* [0][1][2][3]: "EFFE"
		* [4][5][6][7]: Entity's UID.
		* [8][9][10][11]: Entity's effects.
		*/

		strcpy((char*)net_packet->data, "EFFE");
		SDLNet_Write32(static_cast<Uint32>(getUID()), &net_packet->data[4]);
		net_packet->data[8] = 0;
		net_packet->data[9] = 0;
		net_packet->data[10] = 0;
		net_packet->data[11] = 0;
		for ( int i = 0; i < NUMEFFECTS; ++i )
		{
			if ( myStats->EFFECTS[i] )
			{
				net_packet->data[8 + i / 8] |= power(2, i - (i / 8) * 8);
			}
		}
		net_packet->address.host = net_clients[player - 1].host;
		net_packet->address.port = net_clients[player - 1].port;
		net_packet->len = 12;
		if ( guarantee )
		{
			sendPacketSafe(net_sock, -1, net_packet, player - 1);
		}
		else
		{
			sendPacket(net_sock, -1, net_packet, player - 1);
		}
		clientsHaveItsStats = true;
	}
}

bool Entity::setEffect(int effect, bool value, int duration, bool updateClients, bool guarantee)
{
	Stat* myStats = getStats();

	if ( !myStats )
	{
		return false;
	}

	switch ( effect )
	{
		case EFF_ASLEEP:
		case EFF_PARALYZED:
		case EFF_PACIFY:
			if ( (myStats->type >= LICH && myStats->type < KOBOLD)
				|| myStats->type == COCKATRICE || myStats->type == LICH_FIRE || myStats->type == LICH_ICE )
			{
				if ( !(effect == EFF_PACIFY && myStats->type == SHOPKEEPER) )
				{
					return false;
				}
			}
			break;
		case EFF_POLYMORPH:
			//if ( myStats->EFFECTS[EFF_POLYMORPH] || effectPolymorph != 0 )
			//{
			//	return false;
			//}
			break;
		default:
			break;
	}
	myStats->EFFECTS[effect] = value;
	myStats->EFFECTS_TIMERS[effect] = duration;

	int player = -1;
	for ( int i = 0; i < numplayers; ++i )
	{
		if ( players[i]->entity == this )
		{
			player = i;
			break;
		}
	}

	if ( multiplayer == SERVER && player > 0 )
	{
		serverUpdateEffects(player);
	}

	if ( updateClients )
	{
		serverUpdateEffectsForEntity(guarantee);
	}
	return true;
}

void Entity::giveClientStats()
{
	if ( !clientStats )
	{
		clientStats = new Stat(0);
	}
}

void Entity::monsterAcquireAttackTarget(const Entity& target, Sint32 state, bool monsterWasHit)
{
	Stat* myStats = getStats();
	if ( !myStats )
	{
		return;
	}

	bool hadOldTarget = (uidToEntity(monsterTarget) != nullptr);

	if ( &target != uidToEntity(monsterTarget) && !monsterReleaseAttackTarget() )
	{
		//messagePlayer(clientnum, "Entity failed to acquire target!");
		return;
	}

	/*if ( &target != uidToEntity(monsterTarget) )
	{
		messagePlayer(clientnum, "Entity acquired new target!");
	}*/

	if ( myStats->type == LICH_ICE ) // make sure automatons don't attack the leader and vice versa...
	{
		Stat* targetStats = target.getStats();
		if ( targetStats )
		{
			if ( targetStats->type == AUTOMATON && !strncmp(targetStats->name, "corrupted automaton", 19) )
			{
				return;
			}
		}
	}
	else if ( myStats->type == AUTOMATON && !strncmp(myStats->name, "corrupted automaton", 19) )
	{
		if ( target.getRace() == LICH_ICE )
		{
			return;
		}
	}

	if ( monsterState != MONSTER_STATE_ATTACK && !hadOldTarget )
	{
		if ( myStats->type != LICH_FIRE 
			&& myStats->type != LICH_ICE
			&& (myStats->type < LICH || myStats->type > DEVIL)
			)
		{
			// check to see if holding ranged weapon, set hittime to be ready to attack.
			// set melee hittime close to max in hardcore mode...
			if ( ((svFlags & SV_FLAG_HARDCORE) || hasRangedWeapon()) && monsterSpecialTimer <= 0 )
			{
				if ( hasRangedWeapon() )
				{
					monsterHitTime = 2 * HITRATE - 2;
				}
				else if ( svFlags & SV_FLAG_HARDCORE )
				{
					if ( monsterWasHit ) // retaliating to an attack
					{
						monsterHitTime = HITRATE - 12; // 240 ms reaction time
					}
					else // monster find enemy in line of sight
					{
						monsterHitTime = HITRATE - 30; // 600 ms reaction time
					}
				}
			}
		}
	}

	if ( (myStats->type == LICH_FIRE || myStats->type == LICH_ICE)
		&& (monsterState == MONSTER_STATE_LICHFIRE_TELEPORT_STATIONARY 
			|| monsterState == MONSTER_STATE_LICHICE_TELEPORT_STATIONARY
			|| monsterState == MONSTER_STATE_LICH_CASTSPELLS
			|| monsterState == MONSTER_STATE_LICH_TELEPORT_ROAMING
			|| monsterState == MONSTER_STATE_LICHFIRE_DIE
			|| monsterState == MONSTER_STATE_LICHICE_DIE) )
	{

	}
	else
	{
		monsterState = state;
	}
	monsterTarget = target.getUID();
	monsterTargetX = target.x;
	monsterTargetY = target.y;

	if ( target.getStats() != nullptr )
	{
		if ( monsterState != MONSTER_STATE_ATTACK && state == MONSTER_STATE_PATH )
		{
			if ( myStats->type != LICH_FIRE && myStats->type != LICH_ICE && myStats->type != LICH && myStats->type != DEVIL )
			{
				real_t distance = pow(x - target.x, 2) + pow(y - target.y, 2);
				if ( distance < STRIKERANGE * STRIKERANGE )
				{
					monsterState = MONSTER_STATE_ATTACK;
				}
			}
		}
	}

	if ( monsterAllyIndex > 0 && monsterAllyIndex < MAXPLAYERS )
	{
		serverUpdateEntitySkill(this, 1); // update monsterTarget for player leaders.
	}

	if ( !hadOldTarget && myStats->type == SHADOW )
	{
		//messagePlayer(clientnum, "TODO: Shadow got new target.");
		//Activate special ability initially for Shadow.
		monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_SHADOW_TELEMIMICINVISI_ATTACK;
		//pose = MONSTER_POSE_MAGIC_WINDUP1;
		monsterShadowInitialMimic = 1; //true!
		attack(MONSTER_POSE_MAGIC_WINDUP3, 0, nullptr);
	}
}

bool Entity::monsterReleaseAttackTarget(bool force)
{
	if ( !monsterTarget )
	{
		return true;
	}

	Stat* myStats = getStats();
	if ( !myStats )
	{
		return false;
	}

	if ( !force && myStats->type == SHADOW && monsterTarget && uidToEntity(monsterTarget) )
	{
		//messagePlayer(clientnum, "Shadow cannot lose target until it's dead!");
		return false; //Shadow cannot lose its target.
	}

	/*if ( myStats->type == SHADOW )
	{
		messagePlayer(0, "DEBUG: Shadow: Entity::monsterReleaseAttackTarget().");
	}*/

	monsterTarget = 0;

	if ( monsterAllyIndex > 0 && monsterAllyIndex < MAXPLAYERS )
	{
		serverUpdateEntitySkill(this, 1); // update monsterTarget for player leaders.
	}

	return true;
}

void Entity::checkGroundForItems()
{
	Stat* myStats = getStats();
	if ( myStats == nullptr )
	{
		return;
	}
	if ( monsterAllyPickupItems == ALLY_PICKUP_NONE && monsterAllyIndex >= 0 )
	{
		return; // set to ignore ground items.
	}

	// Calls the function for a monster to pick up an item, if it's a monster that picks up items, only if they are not Asleep
	if ( myStats->EFFECTS[EFF_ASLEEP] == false )
	{
		switch ( myStats->type )
		{
			case GOBLIN:
			case HUMAN:
				if ( !strcmp(myStats->name, "") )
				{
					//checkBetterEquipment(myStats);
					monsterAddNearbyItemToInventory(myStats, 16, 9);
				}
				break;
			case GOATMAN:
				//Goatman boss picks up items too.
				monsterAddNearbyItemToInventory(myStats, 16, 9); //Replaces checkBetterEquipment(), because more better. Adds items to inventory, and swaps out current equipped with better stuff on the ground.
																 //checkBetterEquipment(myStats);
				break;
			case AUTOMATON:
				monsterAddNearbyItemToInventory(myStats, 16, 5);
				break;
			default:
				return;
		}
	}
}

bool Entity::canWieldItem(const Item& item) const
{
	Stat* myStats = getStats();
	if ( !myStats )
	{
		return false;
	}

	switch ( myStats->type )
	{
		case GOBLIN:
			return goblinCanWieldItem(item);
		case HUMAN:
			return humanCanWieldItem(item);
		case GOATMAN:
			return goatmanCanWieldItem(item);
		case AUTOMATON:
			return automatonCanWieldItem(item);
		case SHADOW:
			return shadowCanWieldItem(item);
		default:
			return false;
	}
}

void Entity::monsterAddNearbyItemToInventory(Stat* myStats, int rangeToFind, int maxInventoryItems, Entity* forcePickupItem)
{
	//TODO: Any networking/multiplayer needs?
	if ( !myStats )
	{
		return; //Can't continue without these.
	}

	if ( list_Size(&myStats->inventory) >= maxInventoryItems + 1 )
	{
		return;
	}

	list_t* itemsList = nullptr;
	//X and Y in terms of tiles.
	if ( forcePickupItem != nullptr && forcePickupItem->behavior == &actItem )
	{
		if ( !FollowerMenu.allowedInteractItems(myStats->type) )
		{
			return;
		}
		
		//If this is the first item found, the list needs to be created.
		if ( !(itemsList) )
		{
			itemsList = (list_t*)malloc(sizeof(list_t));
			(itemsList)->first = NULL;
			(itemsList)->last = NULL;
		}

		//Add the current entity to it.
		node_t* node2 = list_AddNodeLast(itemsList);
		node2->element = forcePickupItem;
		node2->deconstructor = &emptyDeconstructor;
	}
	else
	{
		int tx = x / 16;
		int ty = y / 16;
		getItemsOnTile(tx, ty, &itemsList); //Check the tile the monster is on for items.
		getItemsOnTile(tx - 1, ty, &itemsList); //Check tile to the left.
		getItemsOnTile(tx + 1, ty, &itemsList); //Check tile to the right.
		getItemsOnTile(tx, ty - 1, &itemsList); //Check tile up.
		getItemsOnTile(tx, ty + 1, &itemsList); //Check tile down.
		getItemsOnTile(tx - 1, ty - 1, &itemsList); //Check tile diagonal up left.
		getItemsOnTile(tx + 1, ty - 1, &itemsList); //Check tile diagonal up right.
		getItemsOnTile(tx - 1, ty + 1, &itemsList); //Check tile diagonal down left.
		getItemsOnTile(tx + 1, ty + 1, &itemsList); //Check tile diagonal down right.
	}
	node_t* node = nullptr;

	if ( itemsList )
	{
		/*
		* Rundown of the function:
		* Loop through all items.
		* Add item to inventory.
		*/

		for ( node = itemsList->first; node != nullptr; node = node->next )
		{
			//Turn the entity into an item.
			if ( node->element )
			{
				if ( list_Size(&myStats->inventory) >= maxInventoryItems + 1 )
				{
					break;
				}

				Entity* entity = (Entity*)node->element;
				if ( entity->flags[INVISIBLE] )
				{
					continue; // ignore invisible items like Sokoban gloves or other scripted events.
				}

				Item* item = nullptr;
				if ( entity != nullptr )
				{
					item = newItemFromEntity(entity);
					if ( forcePickupItem )
					{
						item->forcedPickupByPlayer = true;
					}
				}
				if ( !item )
				{
					continue;
				}

				double dist = sqrt(pow(this->x - entity->x, 2) + pow(this->y - entity->y, 2));
				if ( std::floor(dist) > rangeToFind )
				{
					// item was too far away, continue.
					if ( item != nullptr )
					{
						free(item);
					}
					continue;
				}

				if ( !entity->itemNotMoving && entity->parent && entity->parent != uid )
				{
					if ( itemCategory(item) == THROWN && entity->parent && entity->parent == uid )
					{
						//It's good. Can pick this one up, it's your THROWN now.
					}
					else
					{
						//Don't pick up non-THROWN items that are moving, or owned THROWN that are moving.
						if ( item != nullptr )
						{
							free(item);
						}
						continue; //Item still in motion, don't pick it up.
					}
				}

				Item** shouldWield = nullptr;
				node_t* replaceInventoryItem = nullptr;
				if ( !monsterWantsItem(*item, shouldWield, replaceInventoryItem) )
				{
					if ( item && item->interactNPCUid == getUID() && forcePickupItem )
					{
						// I don't want this.
						handleNPCInteractDialogue(*myStats, ALLY_EVENT_INTERACT_ITEM_NOUSE);
					}
					if ( item != nullptr )
					{
						free(item);
					}
					continue;
				}

				int playerOwned = -1;
				if ( entity->itemOriginalOwner != 0 )
				{
					for ( int c = 0; c < MAXPLAYERS; ++c )
					{
						if ( players[c] && players[c]->entity )
						{
							if ( players[c]->entity->getUID() == entity->itemOriginalOwner )
							{
								if ( players[c]->entity->checkFriend(this) )
								{
									// player owned.
									playerOwned = c;
								}
								break;
							}
						}
					}
					if ( item->interactNPCUid == getUID() )
					{
						// item being interacted with, can interact with item.
					}
					else if ( (playerOwned >= 0 
						&& (entity->ticks < 5 * TICKS_PER_SECOND 
							|| (monsterAllyPickupItems != ALLY_PICKUP_ALL && monsterAllyIndex >= 0)) 
							) 
						)
					{
						// player item too new on ground, or monster is set to not pickup player items.
						continue;
					}
				}

				if ( myStats->type == SLIME )
				{
					if ( item->identified )
					{
						messagePlayer(monsterAllyIndex, language[3145], items[item->type].name_identified);
					}
					else
					{
						messagePlayer(monsterAllyIndex, language[3145], items[item->type].name_unidentified);
					}
					list_RemoveNode(entity->mynode); // slimes eat the item up.
				}
				else if ( shouldWield )
				{
					if ( (*shouldWield) && (*shouldWield)->beatitude < 0 )
					{
						if ( item && item->interactNPCUid == getUID() && forcePickupItem )
						{
							// held item is cursed!
							handleNPCInteractDialogue(*myStats, ALLY_EVENT_INTERACT_ITEM_CURSED);
						}
						if ( item != nullptr )
						{
							free(item);
						}
						continue;
					}

					if ( myStats->type == AUTOMATON && list_Size(&myStats->inventory) < maxInventoryItems
						&& !(item->interactNPCUid == getUID() && forcePickupItem) )
					{
						addItemToMonsterInventory(*shouldWield); // Automatons are hoarders, except if commanded.
					}
					else
					{
						Entity* dropped = dropItemMonster((*shouldWield), this, myStats); //And I threw it on the ground!
						if ( dropped && item && item->interactNPCUid == getUID() )
						{
							dropped->itemOriginalOwner = monsterAllyIndex;
						}
					}

					if ( playerOwned >= 0 && checkFriend(players[playerOwned]->entity)
						&& (item->type >= ARTIFACT_SWORD && item->type <= ARTIFACT_GLOVES) )
					{
						steamAchievementClient(playerOwned, "BARONY_ACH_EARN_THIS");
					}

					if ( item && item->interactNPCUid == getUID() && forcePickupItem )
					{
						if ( itemCategory(item) == AMULET || itemCategory(item) == RING )
						{
							playSoundEntity(this, 33 + rand() % 2, 64);
						}
						else if ( itemCategory(item) == WEAPON || itemCategory(item) == THROWN )
						{
							playSoundEntity(this, 40 + rand() % 4, 64);
						}
						else if ( itemCategory(item) == ARMOR )
						{
							playSoundEntity(this, 44 + rand() % 3, 64);
						}
						else if ( item->type == TOOL_TORCH || item->type == TOOL_LANTERN || item->type == TOOL_CRYSTALSHARD )
						{
							playSoundEntity(this, 134, 64);
						}
					}

					(*shouldWield) = item;
					item = nullptr;
					list_RemoveNode(entity->mynode);
				}
				else if ( replaceInventoryItem )
				{
					//Drop that item out of the monster's inventory, and add this item to the monster's inventory.
					Item* itemToDrop = static_cast<Item*>(replaceInventoryItem->element);
					if ( itemToDrop )
					{
						if ( !(myStats->type == AUTOMATON && list_Size(&myStats->inventory) < maxInventoryItems) )
						{
							// Automatons are hoarders when swapping. Everything else will drop the weapon.
							dropItemMonster(itemToDrop, this, myStats, itemToDrop->count);
						}
						//list_RemoveNode(replaceInventoryItem);
					}

					if ( list_Size(&myStats->inventory) < maxInventoryItems )
					{
						addItemToMonsterInventory(item);
					}
					item = nullptr;
					list_RemoveNode(entity->mynode);
				}
				else if ( list_Size(&myStats->inventory) < maxInventoryItems )
				{
					addItemToMonsterInventory(item);
					item = nullptr;
					list_RemoveNode(entity->mynode);
				}

				if ( item != nullptr )
				{
					free(item);
				}
			}
		}
		list_FreeAll(itemsList);
		free(itemsList);
	}
}

node_t* Entity::addItemToMonsterInventory(Item* item)
{
	//TODO: Sort into inventory...that is, if an item of this type already exists and they can stack, stack 'em instead of creating a new node.
	if ( !item )
	{
		return nullptr;
	}

	Stat* myStats = getStats();
	if ( !myStats )
	{
		return nullptr;
	}

	item->node = list_AddNodeLast(&myStats->inventory);
	if ( !item->node )
	{
		return nullptr;
	}
	item->node->element = item;
	item->node->deconstructor = &defaultDeconstructor;
	item->node->size = sizeof(Item);

	return item->node;
}

bool Entity::shouldMonsterEquipThisWeapon(const Item& itemToEquip) const
{
	Stat* myStats = getStats();
	if ( !myStats )
	{
		return false;
	}

	if ( myStats->weapon == nullptr )
	{
		return true; //Something is better than nothing.
	}

	if ( itemToEquip.interactNPCUid == getUID() )
	{
		return true;
	}

	if ( myStats->weapon->beatitude < 0 )
	{
		//If monster already holding an item, can't swap it out if it's cursed.
		return false;
	}

	if ( myStats->weapon->forcedPickupByPlayer == true )
	{
		return false;
	}

	//Monster is already holding a weapon.
	if ( monsterAllyIndex >= 0 )
	{
		if ( monsterAllyClass == ALLY_CLASS_RANGED && isRangedWeapon(itemToEquip)
			&& !isRangedWeapon(*(myStats->weapon)) )
		{
			// drop what you're holding and pickup that new bow!
			return true;
		}
		else if ( monsterAllyClass == ALLY_CLASS_MELEE && !isRangedWeapon(itemToEquip)
			&& isRangedWeapon(*(myStats->weapon)) )
		{
			// drop what you're holding and pickup that new non-bow!
			return true;
		}
	}

	if ( !Item::isThisABetterWeapon(itemToEquip, myStats->weapon) )
	{
		return false; //Don't want junk.
	}

	if ( itemCategory(myStats->weapon) == MAGICSTAFF || itemCategory(myStats->weapon) == POTION || itemCategory(myStats->weapon) == THROWN || itemCategory(myStats->weapon) == GEM )
	{
		//If current hand item is not cursed, but it's a certain item, don't want to equip this new one.
		return false;
	}

	return true;
}

bool Entity::monsterWantsItem(const Item& item, Item**& shouldEquip, node_t*& replaceInventoryItem) const
{
	Stat* myStats = getStats();
	if ( !myStats )
	{
		return false;
	}

	if ( item.status == BROKEN )
	{
		return false; // no want broken.
	}

	switch ( myStats->type )
	{
		case GOBLIN:
			if ( !goblinCanWieldItem(item) )
			{
				return false;
			}
			break;
		case HUMAN:
			if ( !humanCanWieldItem(item) )
			{
				return false;
			}
			break;
		case GOATMAN:
			if ( !goatmanCanWieldItem(item) )
			{
				return false;
			}
			break;
		case AUTOMATON:
			if ( !automatonCanWieldItem(item) )
			{
				if ( item.interactNPCUid == getUID() )
				{
					// item is being interacted with but we won't auto pick up on interact.
					return false;
				}
				else
				{
					return true; //Can pick up all items automaton can't equip, because recycler.
				}
			}
			break;
		case GNOME:
		case KOBOLD:
		case INCUBUS:
		case INSECTOID:
		case SKELETON:
		case VAMPIRE:
			if ( !monsterAllyEquipmentInClass(item) )
			{
				return false;
			}
			break;
		case SLIME:
			return true; // noms on all items.
			break;
		default:
			return false;
			break;
	}

	switch ( itemCategory(&item) )
	{
		case WEAPON:
			if ( !myStats->weapon )
			{
				shouldEquip = &myStats->weapon;
			}

			if ( myStats->weapon && itemCategory(myStats->weapon) == WEAPON && shouldMonsterEquipThisWeapon(item) )
			{
				shouldEquip = &myStats->weapon;
				return true;
			}
			else
			{
				if ( myStats->weapon && itemCategory(myStats->weapon) == WEAPON )
				{
					//Weapon ain't better than weapon already holding. Don't want it.
					if ( myStats->type == AUTOMATON ) // Automatons are hoarders.
					{
						return true;
					}
					return false;
				}

				if ( monsterAllyIndex >= 0 && monsterAllyIndex < MAXPLAYERS )
				{
					if ( myStats->weapon && item.interactNPCUid == getUID() )
					{
						shouldEquip = &myStats->weapon;
						return true;
					}
					if ( myStats->weapon && myStats->weapon->forcedPickupByPlayer )
					{
						return false;
					}
				}

				//Not holding a weapon. Make sure don't already have a weapon in the inventory. If doesn't have a weapon at all, then add it into the inventory since something is better than nothing.
				node_t* weaponNode = itemNodeInInventory(myStats, static_cast<ItemType>(-1), WEAPON);
				if ( !weaponNode )
				{
					//If no weapons found in inventory, then yes, the goatman wants it, and it should be added to the inventory.
					return true; //Want this item.
				}

				//Search inventory and replace weapon if this one is better.
				if ( Item::isThisABetterWeapon(item, static_cast<Item*>(weaponNode->element)) )
				{
					replaceInventoryItem = weaponNode;
					return true;
				}
				return false; //Don't want your junk.
			}
		case ARMOR:
			if ( myStats->type == AUTOMATON ) // Automatons are hoarders.
			{
				shouldEquip = shouldMonsterEquipThisArmor(item);
				return true;
			}

			return (shouldEquip = shouldMonsterEquipThisArmor(item));
		case THROWN:
			if ( myStats->weapon == nullptr )
			{
				shouldEquip = &myStats->weapon;
				return true;
			}
			else
			{
				return true; //Store in inventory.
			}
		case MAGICSTAFF:
			if ( item.interactNPCUid == getUID() )
			{
				shouldEquip = &myStats->weapon;
			}
			return true;
			break;
		case RING:
			if ( item.interactNPCUid == getUID() )
			{
				shouldEquip = &myStats->ring;
			}
			return true;
			break;
		case AMULET:
			if ( item.interactNPCUid == getUID() )
			{
				shouldEquip = &myStats->amulet;
			}
			return true;
			break;
		case TOOL:
			if ( item.interactNPCUid == getUID() )
			{
				if ( item.type == TOOL_TORCH || item.type == TOOL_LANTERN || item.type == TOOL_CRYSTALSHARD )
				{
					shouldEquip = &myStats->shield;
					return true;
				}
			}
			break;
		default:
			return true; //Already checked if monster likes this specific item in the racial calls.
	}

	return false;
}

Item** Entity::shouldMonsterEquipThisArmor(const Item& item) const
{
	Stat* myStats = getStats();
	if ( !myStats )
	{
		return nullptr;
	}

	switch ( checkEquipType(&item) )
	{
		case TYPE_HAT:
			if ( item.interactNPCUid == getUID() && myStats->helmet )
			{
				return &myStats->helmet;
			}
			if ( myStats->helmet && (myStats->helmet->beatitude < 0 || myStats->helmet->forcedPickupByPlayer == true) )
			{
				return nullptr; //No can has hats : (
			}

			return Item::isThisABetterArmor(item, myStats->helmet) ? &myStats->helmet : nullptr;
		case TYPE_HELM:
			if ( item.interactNPCUid == getUID() && myStats->helmet )
			{
				return &myStats->helmet;
			}

			if ( myStats->helmet && (myStats->helmet->beatitude < 0 || myStats->helmet->forcedPickupByPlayer == true) )
			{
				return nullptr; //Can't swap out armor piece if current one is cursed!
			}

			if ( myStats->type == GOBLIN && myStats->helmet && checkEquipType(myStats->helmet) == TYPE_HAT )
			{
				return nullptr; //Goblins love hats.
			}

			return Item::isThisABetterArmor(item, myStats->helmet) ? &myStats->helmet : nullptr;
			break;
		case TYPE_SHIELD:
			if ( item.interactNPCUid == getUID() && myStats->shield )
			{
				return &myStats->shield;
			}

			if ( myStats->shield && (myStats->shield->beatitude < 0 || myStats->shield->forcedPickupByPlayer == true) )
			{
				return nullptr; //Can't swap out armor piece if current one is cursed!
			}

			return Item::isThisABetterArmor(item, myStats->shield) ? &myStats->shield : nullptr;
		case TYPE_BREASTPIECE:
			if ( item.interactNPCUid == getUID() && myStats->breastplate )
			{
				return &myStats->breastplate;
			}

			if ( myStats->breastplate && (myStats->breastplate->beatitude < 0 || myStats->breastplate->forcedPickupByPlayer == true) )
			{
				return nullptr; //Can't swap out armor piece if current one is cursed!
			}

			return Item::isThisABetterArmor(item, myStats->breastplate) ? &myStats->breastplate : nullptr;
		case TYPE_CLOAK:
			if ( item.interactNPCUid == getUID() && myStats->cloak )
			{
				return &myStats->cloak;
			}

			if ( myStats->cloak && (myStats->cloak->beatitude < 0 || myStats->cloak->forcedPickupByPlayer == true) )
			{
				return nullptr; //Can't swap out armor piece if current one is cursed!
			}

			return Item::isThisABetterArmor(item, myStats->cloak) ? &myStats->cloak : nullptr;
		case TYPE_BOOTS:
			if ( item.interactNPCUid == getUID() && myStats->shoes )
			{
				return &myStats->shoes;
			}

			if ( myStats->shoes && (myStats->shoes->beatitude < 0 || myStats->shoes->forcedPickupByPlayer == true) )
			{
				return nullptr; //Can't swap out armor piece if current one is cursed!
			}

			return Item::isThisABetterArmor(item, myStats->shoes) ? &myStats->shoes : nullptr;
		case TYPE_GLOVES:
			if ( item.interactNPCUid == getUID() && myStats->gloves )
			{
				return &myStats->gloves;
			}

			if ( myStats->gloves && (myStats->gloves->beatitude < 0 || myStats->gloves->forcedPickupByPlayer == true) )
			{
				return nullptr; //Can't swap out armor piece if current one is cursed!
			}

			return Item::isThisABetterArmor(item, myStats->gloves) ? &myStats->gloves : nullptr;
			break;
		default:
			return nullptr;
	}
}

double Entity::monsterRotate()
{
	double dir = yaw - monsterLookDir;
	while ( dir >= PI )
	{
		dir -= PI * 2;
	}
	while ( dir < -PI )
	{
		dir += PI * 2;
	}
	yaw -= dir / 2;
	while ( yaw < 0 )
	{
		yaw += 2 * PI;
	}
	while ( yaw >= 2 * PI )
	{
		yaw -= 2 * PI;
	}

	return dir;
}

Item* Entity::getBestMeleeWeaponIHave() const
{
	Stat* myStats = getStats();
	if ( !myStats )
	{
		return nullptr;
	}

	Item* currentBest = nullptr;
	if ( myStats->weapon && isMeleeWeapon(*myStats->weapon) )
	{
		currentBest = myStats->weapon;
	}

	//Loop through the creature's inventory & find the best item. //TODO: Make it work on multiplayer clients?
	for ( node_t* node = myStats->inventory.first; node; node = node->next )
	{
		Item* item = static_cast<Item*>(node->element);
		if ( item )
		{
			if ( isMeleeWeapon(*item) && Item::isThisABetterWeapon(*item, currentBest) )
			{
				currentBest = item;
			}
		}
	}

	/*if ( currentBest )
	{
		messagePlayer(clientnum, "Found best melee weapon: \"%s\"", currentBest->description());
	}*/

	return currentBest;
}

Item* Entity::getBestShieldIHave() const
{
	Stat* myStats = getStats();
	if ( !myStats )
	{
		return nullptr;
	}

	Item* currentBest = nullptr;
	if ( myStats->shield && myStats->shield->isShield() )
	{
		currentBest = myStats->shield;
	}

	//Loop through the creature's inventory & find the best item. //TODO: Make it work on multiplayer clients?
	for ( node_t* node = myStats->inventory.first; node; node = node->next )
	{
		Item* item = static_cast<Item*>(node->element);
		if ( item )
		{
			if ( item->isShield() && Item::isThisABetterArmor(*item, currentBest) )
			{
				currentBest = item;
			}
		}
	}

	/*if ( currentBest )
	{
		messagePlayer(clientnum, "Found best shield: \"%s\"", currentBest->description());
	}*/

	return currentBest;
}

void Entity::degradeArmor(Stat& hitstats, Item& armor, int armornum)
{
	if ( hitstats.type == SHADOW )
	{
		return; //Shadows' armor and shields don't break.
	}

	if ( hitstats.type == SKELETON && monsterAllySummonRank > 0 )
	{
		return; // conjured skeleton armor doesn't break.
	}

	if ( armor.type == ARTIFACT_BOOTS
		|| armor.type == ARTIFACT_HELM
		|| armor.type == ARTIFACT_CLOAK
		|| armor.type == ARTIFACT_GLOVES
		|| armor.type == ARTIFACT_BREASTPIECE )
	{
		return;
	}

	int playerhit = -1;

	if ( this->behavior == &actPlayer )
	{
		playerhit = this->skill[2];
	}

	if ( playerhit == clientnum || playerhit < 0 )
	{
		if ( armor.count > 1 )
		{
			newItem(armor.type, armor.status, armor.beatitude, armor.count - 1, armor.appearance, armor.identified, &hitstats.inventory);
		}
	}
	armor.count = 1;
	armor.status = static_cast<Status>(std::max(static_cast<int>(BROKEN), armor.status - 1));
	if ( armor.status > BROKEN )
	{
		if ( armor.type == TOOL_CRYSTALSHARD )
		{
			messagePlayer(playerhit, language[2350], armor.getName());
		}
		else
		{
			messagePlayer(playerhit, language[681], armor.getName());
		}
	}
	else
	{
		if ( armor.type == TOOL_CRYSTALSHARD )
		{
			playSoundEntity(hit.entity, 162, 64);
			messagePlayer(playerhit, language[2351], armor.getName());
		}
		else
		{
			playSoundEntity(hit.entity, 76, 64);
			messagePlayer(playerhit, language[682], armor.getName());
		}
	}
	if ( playerhit > 0 && multiplayer == SERVER )
	{
		strcpy((char*)net_packet->data, "ARMR");
		net_packet->data[4] = armornum;
		net_packet->data[5] = armor.status;
		net_packet->address.host = net_clients[playerhit - 1].host;
		net_packet->address.port = net_clients[playerhit - 1].port;
		net_packet->len = 6;
		sendPacketSafe(net_sock, -1, net_packet, playerhit - 1);
	}
}

void Entity::removeLightField()
{
	if ( this->light != nullptr )
	{
		list_RemoveNode(this->light->node);
		this->light = nullptr;
	}
}

bool Entity::shouldRetreat(Stat& myStats)
{
	// monsters that retreat based on CHR
	// gnomes, spiders, goblins, shopkeeps, trolls, humans (50%)
	// kobolds, scarabs, vampires, suc/incubi, insectoids, goatmen, rats

	// excluded golems, shadows, cockatrice, skeletons, demons, imps
	// scorpions, slimes, ghouls

	// retreating monsters will not try path when losing sight of target

	if ( myStats.EFFECTS[EFF_PACIFY] )
	{
		return true;
	}
	if ( myStats.type == SHADOW )
	{
		return false;
	}
	else if ( myStats.type == LICH_FIRE )
	{
		if ( monsterLichFireMeleeSeq == LICH_ATK_BASICSPELL_SINGLE )
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	if ( monsterAllySummonRank != 0 )
	{
		return false;
	}
	if ( myStats.type == LICH_ICE )
	{
		return false;
	}

	Entity* leader = monsterAllyGetPlayerLeader();
	if ( leader && stats[monsterAllyIndex] )
	{
		Stat* leaderStats = leader->getStats();
		if ( leaderStats->PROFICIENCIES[PRO_LEADERSHIP] + statGetCHR(stats[monsterAllyIndex]) >= AllyNPCSkillRequirements[ALLY_CMD_ATTACK_CONFIRM] )
		{
			// do not retreat for brave leader!
			return false;
		}
	}

	if ( myStats.MAXHP >= 100 )
	{
		if ( myStats.HP <= myStats.MAXHP / 6 && this->getCHR() >= -2 )
		{
			return true;
		}
	}
	else if ( myStats.HP <= myStats.MAXHP / 3 && this->getCHR() >= -2 )
	{
		return true;
	}

	return false;
}

bool Entity::backupWithRangedWeapon(Stat& myStats, int dist, int hasrangedweapon)
{
	if ( dist >= 100 || !hasrangedweapon )
	{
		return false;
	}

	if ( myStats.type == INSECTOID && monsterSpecialState > 0 )
	{
		return false;
	}
	if ( myStats.type == VAMPIRE && (monsterSpecialState > 0 || !strncmp(myStats.name, "Bram Kindly", 11)) )
	{
		return false;
	}

	return true;
}

void Entity::monsterEquipItem(Item& item, Item** slot)
{
	if ( !slot )
	{
		return;
	}

	Stat *myStats = getStats();
	if ( !myStats )
	{
		return;
	}

	dropItemMonster((*slot), this, myStats);

	*slot = &item;
}

bool Entity::monsterHasSpellbook(int spellbookType)
{
	if (spellbookType == SPELL_NONE )
	{
		//messagePlayer(clientnum, "[DEBUG: Entity::monsterHasSpellbook()] skipping SPELL_NONE");
		return false;
	}

	Stat* myStats = getStats();
	if ( !myStats )
	{
		return false;
	}

	if ( myStats->weapon && getSpellIDFromSpellbook(myStats->weapon->type) == spellbookType )
	{
		spell_t *spell = getSpellFromID(getSpellIDFromSpellbook(myStats->weapon->type));
		//messagePlayer(clientnum, "DEBUG: Monster has spell %s.", spell->name);
		return true;
	}

	for ( node_t* node = myStats->inventory.first; node; node = node->next )
	{
		Item* item = static_cast<Item*>(node->element);
		if ( !item )
		{
			continue;
		}

		if ( getSpellIDFromSpellbook(item->type) == spellbookType )
		{
			spell_t *spell = getSpellFromID(getSpellIDFromSpellbook(item->type));
			//messagePlayer(clientnum, "DEBUG: Monster HAS spell %s.", spell->name);
			return true;
		}
	}

	return false;
}

bool Entity::isSpellcasterBeginner()
{
	Stat* myStats = getStats();
	if ( !myStats )
	{
		return false;
	}
	else if ( myStats->PROFICIENCIES[PRO_SPELLCASTING] < SPELLCASTING_BEGINNER )
	{
		return true; //The caster has lower spellcasting skill. Cue happy fun times.
	}
	return false;
}

char* Entity::getMonsterLangEntry()
{
	Stat* myStats = getStats();
	if ( !myStats )
	{
		return nullptr;
	}
	if ( !strcmp(myStats->name, "") )
	{
		if ( myStats->type < KOBOLD ) //Original monster count
		{
			return language[90 + myStats->type];
		}
		else if ( myStats->type >= KOBOLD ) //New monsters
		{
			return language[2000 + (myStats->type - KOBOLD)];
		}
	}
	else
	{
		return myStats->name;
	}
	return nullptr;
}

void playerStatIncrease(int playerClass, int chosenStats[3])
{
	std::mt19937 seed(rand()); // seed of distribution.
	
	std::vector<int> statWeights = classStatGrowth[playerClass];

	// debug to print which vector values are being used.
	//for ( std::vector<int>::const_iterator i = statWeights.begin(); i != statWeights.end(); ++i )
	//{
	//	messagePlayer(0, "%2d, ", *i);
	//}

	chosenStats[0] = rand() % 6; // get first stat randomly.
	statWeights[chosenStats[0]] = 0; // remove the chance of the local stat vector.

	std::discrete_distribution<> distr2(statWeights.begin(), statWeights.end()); // regen the distribution with new weights.
	chosenStats[1] = distr2(seed); // get second stat.
	statWeights[chosenStats[1]] = 0; // remove the chance in the local stat vector.

	std::discrete_distribution<> distr3(statWeights.begin(), statWeights.end()); // regen the distribution with new weights.
	chosenStats[2] = distr3(seed); // get third stat.

	if ( chosenStats[0] == chosenStats[1] || chosenStats[0] == chosenStats[2] || chosenStats[1] == chosenStats[2] )
	{
		printlog("Err: duplicate stat index chosen on level up of player with class %d!\n", playerClass);
	}

	return;
}

void Entity::createPathBoundariesNPC(int maxTileDistance)
{
	Stat* myStats = this->getStats();

	if ( !myStats )
	{
		return;
	}

	if ( myStats->MISC_FLAGS[STAT_FLAG_NPC] != 0 
		|| myStats->type == SHOPKEEPER
		|| monsterAllyState == ALLY_STATE_DEFEND )
	{
		// is NPC, find the bounds which movement is restricted to by finding the "box" it spawned in.
		int i, j;
		int numTiles = 0;
		monsterPathBoundaryXStart = x / 16;
		monsterPathBoundaryXEnd = x / 16;
		monsterPathBoundaryYStart = y / 16;
		monsterPathBoundaryYEnd = y / 16;
		for ( i = x; i >= 0; i -= 16 )
		{
			if ( !checkObstacle(i, y, this, nullptr) )
			{
				monsterPathBoundaryXStart = i;
			}
			else
			{
				if ( monsterAllyState == ALLY_STATE_DEFEND )
				{
					// don't use players to block boundaries.
					bool foundplayer = false;
					for ( int player = 0; player < MAXPLAYERS; ++player )
					{
						if ( players[player] && players[player]->entity )
						{
							int playerx = static_cast<int>(players[player]->entity->x);
							int playery = static_cast<int>(players[player]->entity->y);
							if ( playerx == i && playery == y )
							{
								monsterPathBoundaryXStart = i;
								foundplayer = true;
							}
						}
					}
					if ( !foundplayer )
					{
						break;
					}
				}
				else
				{
					break;
				}
			}
			if ( maxTileDistance > 0 )
			{
				++numTiles;
				if ( numTiles > maxTileDistance )
				{
					break;
				}
			}
		}
		numTiles = 0;
		for ( i = x; i < map.width << 4; i += 16 )
		{
			if ( !checkObstacle(i, y, this, nullptr) )
			{
				monsterPathBoundaryXEnd = i;
			}
			else
			{
				if ( monsterAllyState == ALLY_STATE_DEFEND )
				{
					// don't use players to block boundaries.
					bool foundplayer = false;
					for ( int player = 0; player < MAXPLAYERS; ++player )
					{
						if ( players[player] && players[player]->entity )
						{
							int playerx = static_cast<int>(players[player]->entity->x);
							int playery = static_cast<int>(players[player]->entity->y);
							if ( playerx == i && playery == y )
							{
								monsterPathBoundaryXEnd = i;
								foundplayer = true;
							}
						}
					}
					if ( !foundplayer )
					{
						break;
					}
				}
				else
				{
					break;
				}
			}
			if ( maxTileDistance > 0 )
			{
				++numTiles;
				if ( numTiles > maxTileDistance )
				{
					break;
				}
			}
		}
		numTiles = 0;
		for ( j = y; j >= 0; j -= 16 )
		{
			if ( !checkObstacle(x, j, this, nullptr) )
			{
				monsterPathBoundaryYStart = j;
			}
			else
			{
				if ( monsterAllyState == ALLY_STATE_DEFEND )
				{
					// don't use players to block boundaries.
					bool foundplayer = false;
					for ( int player = 0; player < MAXPLAYERS; ++player )
					{
						if ( players[player] && players[player]->entity )
						{
							int playerx = static_cast<int>(players[player]->entity->x);
							int playery = static_cast<int>(players[player]->entity->y);
							if ( playerx == x && playery == j )
							{
								monsterPathBoundaryYStart = j;
								foundplayer = true;
							}
						}
					}
					if ( !foundplayer )
					{
						break;
					}
				}
				else
				{
					break;
				}
			}
			if ( maxTileDistance > 0 )
			{
				++numTiles;
				if ( numTiles > maxTileDistance )
				{
					break;
				}
			}
		}
		numTiles = 0;
		for ( j = y; j < map.height << 4; j += 16 )
		{
			if ( !checkObstacle(x, j, this, nullptr) )
			{
				monsterPathBoundaryYEnd = j;
			}
			else
			{
				if ( monsterAllyState == ALLY_STATE_DEFEND )
				{
					// don't use players to block boundaries.
					bool foundplayer = false;
					for ( int player = 0; player < MAXPLAYERS; ++player )
					{
						if ( players[player] && players[player]->entity )
						{
							int playerx = static_cast<int>(players[player]->entity->x);
							int playery = static_cast<int>(players[player]->entity->y);
							if ( playerx == x && playery == j )
							{
								monsterPathBoundaryYEnd = j;
								foundplayer = true;
							}
						}
					}
					if ( !foundplayer )
					{
						break;
					}
				}
				else
				{
					break;
				}
			}
			if ( maxTileDistance > 0 )
			{
				++numTiles;
				if ( numTiles > maxTileDistance )
				{
					break;
				}
			}
		}
		numTiles = 0;
		//messagePlayer(0, "restricted to (%d, %d), (%d, %d)", monsterPathBoundaryXStart >> 4, monsterPathBoundaryYStart >> 4, monsterPathBoundaryXEnd >> 4, monsterPathBoundaryYEnd >> 4);
	}
}

node_t* Entity::chooseAttackSpellbookFromInventory()
{
	Stat* myStats = getStats();
	if (!myStats )
	{
		return nullptr;
	}

	node_t* spellbook = nullptr;
	std::vector<int> spellbooks;

	//Ok, first, compile a list of all spells it has on it.
	//Then choose one and return it.
	for ( int i = 1; i < NUM_SPELLS; ++i ) //Skip 0, which = SPELL_NONE.
	{
		if ( monsterHasSpellbook(i) )
		{
			if ( myStats->type == SHADOW ) //TODO: Replace this if-else block with an "isAttackSpell() && monsterCanUseSpell()"
			{
				if ( shadowCanMimickSpell(i) )
				{
					//messagePlayer(clientnum, "I can mimic spell %d!", i);
					spellbooks.push_back(i);
				}
				else
				{
					//messagePlayer(clientnum, "I no can does spell %d", i);
				}
			}
			else
			{
				//messagePlayer(clientnum, "TODO: Only shadow has CanCastSpell() checking implemented! Need to update other relevant monsters.");
			}
		}
	}

	if ( spellbooks.size() == 0 )
	{
		//messagePlayer(clientnum, "[DEBUG:Entity::chooseAttackSpellbookFromInventory()] No applicable spellbooks on me!");
		return nullptr;
	}

	spellbook = spellbookNodeInInventory(myStats, spellbooks[rand()%spellbooks.size()]); //Choose a random spell and return it.
	if (!spellbook )
	{
		//messagePlayer(clientnum, "[DEBUG:Entity::chooseAttackSpellbookFromInventory()] Error: Failed to choose a spellbook!");
	}
	return spellbook;
}

int Entity::getManaRegenInterval(Stat& myStats)
{
	int regenTime = getBaseManaRegen(myStats);
	int manaring = 0;
	if ( behavior == &actPlayer && myStats.type != HUMAN )
	{
		if ( myStats.type == SKELETON )
		{
			manaring = -1; // 0.25x regen speed.
		}
	}
	if ( myStats.breastplate != nullptr )
	{
		if ( myStats.breastplate->type == VAMPIRE_DOUBLET )
		{
			if ( myStats.breastplate->beatitude >= 0 )
			{
				manaring++;
			}
			else
			{
				manaring--;
			}
		}
	}
	if ( myStats.cloak != nullptr )
	{
		if ( myStats.cloak->type == ARTIFACT_CLOAK )
		{
			if ( myStats.cloak->beatitude >= 0 )
			{
				manaring++;
			}
			else
			{
				manaring--;
			}
		}
	}

	if ( manaring >= 2 && ticks % TICKS_PER_SECOND == 0 )
	{
		steamAchievementEntity(this, "BARONY_ACH_ARCANE_LINK");
	}

	if ( myStats.EFFECTS[EFF_MP_REGEN] )
	{
		manaring += 2;
		if ( manaring > 3 )
		{
			manaring = 3;
		}
	}

	if ( manaring > 0 )
	{
		return regenTime / (manaring * 2); // 1 MP each 6 seconds base
	}
	else if ( manaring < 0 )
	{
		return regenTime * abs(manaring) * 4; // 1 MP each 24 seconds if negative regen
	}
	else if ( manaring == 0 )
	{
		return regenTime;
	}
	return MAGIC_REGEN_TIME;
}

int Entity::getHealthRegenInterval(Stat& myStats)
{
	if ( myStats.EFFECTS[EFF_VAMPIRICAURA] )
	{
		return -1;
	}
	if ( myStats.breastplate && myStats.breastplate->type == VAMPIRE_DOUBLET )
	{
		return -1;
	}
	int healring = 0;
	if ( behavior == &actPlayer && myStats.type != HUMAN )
	{
		if ( myStats.type == SKELETON )
		{
			healring = -1; // 0.25x regen speed.
		}
	}
	if ( myStats.ring != nullptr )
	{
		if ( myStats.ring->type == RING_REGENERATION )
		{
			if ( myStats.ring->beatitude >= 0 )
			{
				healring++;
				healring += std::min(static_cast<int>(myStats.ring->beatitude), 1);
			}
			else
			{
				healring--;
			}
		}
	}
	if ( myStats.breastplate != nullptr )
	{
		if ( myStats.breastplate->type == ARTIFACT_BREASTPIECE )
		{
			if ( myStats.breastplate->beatitude >= 0 )
			{
				healring++;
			}
			else
			{
				healring--;
			}
		}
	}

	if ( healring >= 2 && ticks % TICKS_PER_SECOND == 0 )
	{
		steamAchievementEntity(this, "BARONY_ACH_TROLLS_BLOOD");
	}
	
	if ( myStats.EFFECTS[EFF_HP_REGEN] )
	{
		if ( monsterAllyGetPlayerLeader() && monsterAllySpecial == ALLY_SPECIAL_CMD_REST && myStats.EFFECTS[EFF_ASLEEP] )
		{
			healring += 1;
		}
		else
		{
			healring += 2;
		}
		if ( healring > 3 )
		{
			healring = 3;
		}
	}

	if ( !strncmp(map.name, "Mages Guild", 11) && myStats.type == SHOPKEEPER )
	{
		healring = 25; // these guys like regenerating
	}

	if ( healring > 0 )
	{
		return (HEAL_TIME / (healring * 6)); // 1 HP each 12 sec base
	}
	else if ( healring < 0 )
	{
		return (abs(healring) * HEAL_TIME * 4); // 1 HP each 48 sec if negative regen
	}
	else if ( healring == 0 )
	{
		return HEAL_TIME;
	}
	return HEAL_TIME;
}

int Entity::getBaseManaRegen(Stat& myStats)
{
	// reduced time from intelligence and spellcasting ability, 0-150 ticks of 300.
	int profMultiplier = (myStats.PROFICIENCIES[PRO_SPELLCASTING] / 20) + 1; // 1 to 6
	int statMultiplier = std::max(getINT(), 0); // get intelligence

	return (MAGIC_REGEN_TIME - static_cast<int>(std::min(profMultiplier * statMultiplier, 150))); // return 300-150 ticks, 6-3 seconds.
}

void Entity::setRangedProjectileAttack(Entity& marksman, Stat& myStats)
{
	// get arrow power.
	int attack = marksman.getRangedAttack();
	int chance = (attack / 2) * (100 - myStats.PROFICIENCIES[PRO_RANGED]) / 100.f;
	if ( chance > 0 )
	{
		attack = (attack - chance) + (rand() % chance) + 1;
	}
	this->arrowPower = attack;

	// get arrow effects.
	if ( myStats.weapon )
	{
		if ( myStats.weapon->type == ARTIFACT_BOW )
		{
			// poison arrow
			this->arrowPoisonTime = 540;    // 9 seconds of poison
		}

		if ( myStats.weapon->type != SLING )
		{
			// get armor pierce chance.
			int statChance = std::min(std::max(marksman.getPER() / 2, 0), 50); // 0 to 50 value.
			int chance = rand() % 100;
			if ( chance < statChance )
			{
				this->arrowArmorPierce = 1; // pierce half of armor in damage calc.
			}
			else
			{
				this->arrowArmorPierce = 0;
			}
		}
	}
}

/* SetEntityOnFire
 * Attempts to set the Entity on fire. Entities that are not Burnable or are already on fire will return before any processing
 * Entities that do not have Stats (such as furniture) will return after setting the fire time and chance to stop at max
 * Entities with Stats will have their fire time (char_fire) and chance to stop being on fire (chanceToPutOutFire) reduced by their CON
 * Calculations for reductions is outlined in this function
 */
void Entity::SetEntityOnFire()
{
	// Check if the Entity can be set on fire
	if ( this->flags[BURNABLE] )
	{
		if ( this->behavior == &actPlayer )
		{
			Stat* myStats = this->getStats();
			if ( myStats && myStats->type == SKELETON )
			{
				return;
			}
		}
		// Check if the Entity is already on fire
		if ( !(this->flags[BURNING]) )
		{
			this->flags[BURNING] = true;
			serverUpdateEntityFlag(this, BURNING);

			/* Set the time the Entity will be on fire, based off their CON
			 * |\_ MAX_TICKS_ON_FIRE is reduced by every 2 points in CON
			 * |
			 * |\_ Fire has a minimum of 4 cycles (120 ticks), and a maximum of 20 cycles (600 ticks), cycles are based off of TICKS_TO_PROCESS_FIRE
			 * |  \_ Constants are defined in entity.hpp: MIN_TICKS_ON_FIRE and MAX_TICKS_ON_FIRE
			 * |
			 *  \_ For every 5 points of CON, the chance to stop being on fire is increased
			 *    \_ The chance to stop being on fire has a minimum of 1 in 10, and a maximum of 1 in 5
			 *      \_ Constants are defined in entity.hpp: MIN_CHANCE_STOP_FIRE and MAX_CHANCE_STOP_FIRE
			 */

			// Set the default time on fire
			this->char_fire = MAX_TICKS_ON_FIRE;
			// Set the default chance of putting out fire
			this->chanceToPutOutFire = MAX_CHANCE_STOP_FIRE;

			// If the Entity is not a Monster, it wont have Stats, end here
			if ( this->getStats() == nullptr )
			{
				return; // The Entity was set on fire, it does not have Stats, so it is on fire for maximum duration
			}

			// Determine decrease in time on fire based on the Entity's CON
			const Sint32 entityCON = this->getStats()->CON;

			// If the Entity's CON is <= 1 then their time is just MAX_TICKS_ON_FIRE
			if ( entityCON <= 1 )
			{
				return; // The Entity was set on fire, with maximum duration and chance
			}

			// If the Entity's CON is <= 4 then their chance is just MAX_CHANCE_STOP_FIRE
			if ( entityCON <= 4 )
			{
				this->chanceToPutOutFire = MAX_CHANCE_STOP_FIRE;
			}
			else if ( entityCON >= MAX_CON_FOR_STOP_FIRE ) // If the Entity has MAX_CON_FOR_STOP_FIRE (25) or greater CON, then the reduction is equal to or less than MIN_CHANCE_STOP_FIRE
			{
				this->chanceToPutOutFire = MIN_CHANCE_STOP_FIRE;
			}
			else
			{
				this->chanceToPutOutFire -= static_cast<Sint32>(floor(entityCON * 0.2));
			}

			// If the Entity has MAX_CON_FOR_FIRE_TIME (32) or greater CON, then the reduction is equal or less than MIN_TICKS_ON_FIRE
			if ( entityCON >= MAX_CON_FOR_FIRE_TIME )
			{
				this->char_fire = MIN_TICKS_ON_FIRE;
			}
			else
			{
				this->char_fire -= static_cast<Sint32>(floor((entityCON * 0.5) * TICKS_TO_PROCESS_FIRE));
			}

			return; // The Entity was set on fire, with a reduced duration
		}
	}

	return; // The Entity can/should not be set on fire
}

/*-------------------------------------------------------------------------------

messagePlayerMonsterEvent
handles text for monster interaction/damage/obituaries

-------------------------------------------------------------------------------*/

void messagePlayerMonsterEvent(int player, Uint32 color, Stat& monsterStats, char* msgGeneric, char* msgNamed, int detailType, Entity* monster)
{
	if ( player < 0 || player >= MAXPLAYERS )
	{
		return;
	}

	// If true, pretend the monster doesn't have a name and use the generic message "You hit the lesser skeleton!"
	bool namedMonsterAsGeneric = monsterNameIsGeneric(monsterStats);
	int monsterType = monsterStats.type;
	if ( monster != nullptr && monster->behavior == &actPlayer )
	{
		monsterType = monster->getMonsterTypeFromSprite();
	}

	//char str[256] = { 0 };
	if ( !strcmp(monsterStats.name, "") )
	{
		// use generic racial name and grammar. "You hit the skeleton"
		if ( detailType == MSG_OBITUARY )
		{
			for ( int c = 0; c < MAXPLAYERS; ++c )
			{
				if ( client_disconnected[c] )
				{
					continue;
				}
				if ( c == player )
				{
					if ( monsterType < KOBOLD ) // Original monster count
					{
						messagePlayerColor(c, color, msgNamed, language[90 + monsterType], monsterStats.obituary);
					}
					else if ( monsterType >= KOBOLD ) //New monsters
					{
						messagePlayerColor(c, color, msgNamed, language[2000 + (monsterType - KOBOLD)], monsterStats.obituary);
					}
				}
				else
				{
					if ( monsterType < KOBOLD ) // Original monster count
					{
						messagePlayerColor(c, color, msgGeneric, stats[player]->name, language[90 + monsterType], monsterStats.obituary);
					}
					else if ( monsterType >= KOBOLD ) //New monsters
					{
						messagePlayerColor(c, color, msgGeneric, stats[player]->name, language[2000 + (monsterType - KOBOLD)], monsterStats.obituary);
					}
				}
			}
		}
		else if ( detailType == MSG_ATTACKS )
		{
			if ( monsterType < KOBOLD ) // Original monster count
			{
				messagePlayerColor(player, color, msgGeneric, language[90 + monsterType], language[132 + monsterType]);
			}
			else if ( monsterType >= KOBOLD ) //New monsters
			{
				messagePlayerColor(player, color, msgGeneric, language[2000 + (monsterType - KOBOLD)], language[2100 + (monsterType - KOBOLD)]);
			}
		}
		else
		{
			if ( monsterType < KOBOLD ) // Original monster count
			{
				messagePlayerColor(player, color, msgGeneric, language[90 + monsterType]);
			}
			else if ( monsterType >= KOBOLD ) //New monsters
			{
				messagePlayerColor(player, color, msgGeneric, language[2000 + (monsterType - KOBOLD)]);
			}
		}
	}
	else
	{
		// use monster's "name" and pronoun grammar. "You hit Funny Bones!"
		if ( detailType == MSG_DESCRIPTION )
		{
			if ( namedMonsterAsGeneric )
			{
				messagePlayerColor(player, color, msgGeneric, monsterStats.name);
			}
			else if ( monsterType < KOBOLD ) //Original monster count
			{
				messagePlayerColor(player, color, msgNamed, language[90 + monsterType], monsterStats.name);
			}
			else if ( monsterType >= KOBOLD ) //New monsters
			{
				messagePlayerColor(player, color, msgNamed, language[2000 + (monsterType - KOBOLD)], monsterStats.name);
			}
		}
		else if ( detailType == MSG_COMBAT )
		{
			if ( namedMonsterAsGeneric )
			{
				messagePlayerColor(player, color, msgGeneric, monsterStats.name);
			}
			else if ( monsterType < KOBOLD ) //Original monster count
			{
				messagePlayerColor(player, color, msgNamed, monsterStats.name);
			}
			else if ( monsterType >= KOBOLD ) //New monsters
			{
				messagePlayerColor(player, color, msgNamed, monsterStats.name);
			}
		}
		else if ( detailType == MSG_OBITUARY )
		{
			for ( int c = 0; c < MAXPLAYERS; ++c )
			{
				if ( client_disconnected[c] )
				{
					continue;
				}
				if ( namedMonsterAsGeneric )
				{
					if ( c == player )
					{
						messagePlayerColor(c, color, msgNamed, monsterStats.name, monsterStats.obituary);
					}
					else
					{
						messagePlayerColor(c, color, msgGeneric, stats[player]->name, monsterStats.name, monsterStats.obituary);
					}
				}
				else
				{
					messagePlayerColor(c, color, "%s %s", monsterStats.name, monsterStats.obituary);
				}
			}
		}
		else if ( detailType == MSG_GENERIC )
		{
			if ( namedMonsterAsGeneric || monsterType == HUMAN )
			{
				messagePlayerColor(player, color, msgGeneric, monsterStats.name);
			}
			else if ( monsterType < KOBOLD ) // Original monster count
			{
				messagePlayerColor(player, color, msgGeneric, language[90 + monsterType]);
			}
			else if ( monsterType >= KOBOLD ) //New monsters
			{
				messagePlayerColor(player, color, msgGeneric, language[2000 + (monsterType - KOBOLD)]);
			}
		}
		else if ( detailType == MSG_ATTACKS )
		{
			if ( namedMonsterAsGeneric )
			{
				if ( monsterType < KOBOLD ) // Original monster count
				{
					messagePlayerColor(player, color, msgGeneric, monsterStats.name, language[132 + monsterType]);
				}
				else if ( monsterType >= KOBOLD ) //New monsters
				{
					messagePlayerColor(player, color, msgGeneric, monsterStats.name, language[2100 + (monsterType - KOBOLD)]);
				}
			}
			else if ( monsterType < KOBOLD ) // Original monster count
			{
				messagePlayerColor(player, color, msgNamed, monsterStats.name, language[132 + monsterType]);
			}
			else if ( monsterType >= KOBOLD ) //New monsters
			{
				messagePlayerColor(player, color, msgNamed, monsterStats.name, language[2100 + (monsterType - KOBOLD)]);
			}
		}
	}
}

/*-------------------------------------------------------------------------------

playerClassLangEntry
get text string for the different player chosen classes.

-------------------------------------------------------------------------------*/

char* playerClassLangEntry(int classnum)
{
	if ( classnum >= 0 && classnum <= 9 )
	{
		return language[1900 + classnum];
	}
	else if ( classnum >= 10 && classnum <= NUMCLASSES )
	{
		return language[2550 + classnum - 10];
	}
	else
	{
		return "undefined classname";
	}
}

/*-------------------------------------------------------------------------------

playerClassDescription
get text string for the description of player chosen classes.

-------------------------------------------------------------------------------*/

char* playerClassDescription(int classnum)
{
	if ( classnum >= 0 && classnum <= 9 )
	{
		return language[10 + classnum];
	}
	else if ( classnum >= 10 && classnum <= NUMCLASSES )
	{
		return language[2560 + classnum - 10];
	}
	else
	{
		return "undefined description";
	}
}

/*-------------------------------------------------------------------------------

setHelmetLimbOffset
Adjusts helmet offsets for all monsters, depending on the type of headwear.

-------------------------------------------------------------------------------*/

void Entity::setHelmetLimbOffset(Entity* helm)
{
	// for non-armor helmets, they are rotated so focaly acts as up/down postion.
	int monster = getMonsterTypeFromSprite();
	if ( helm->sprite == items[HAT_PHRYGIAN].index )
	{
		switch ( monster )
		{
			case AUTOMATON:
			case SKELETON:
				helm->focalx = limbs[monster][9][0] - .5;
				helm->focaly = limbs[monster][9][1] - 3.25;
				helm->focalz = limbs[monster][9][2] + 2.25;
				break;
			case HUMAN:
			case SHOPKEEPER:
			case VAMPIRE:
				helm->focalx = limbs[monster][9][0] - .5;
				helm->focaly = limbs[monster][9][1] - 3.25;
				helm->focalz = limbs[monster][9][2] + 2.25;
				break;
			case INSECTOID:
				helm->focalx = limbs[monster][9][0] - .5;
				helm->focaly = limbs[monster][9][1] - 3.05;
				helm->focalz = limbs[monster][9][2] + 2.25;
				break;
			case GOBLIN:
			case SHADOW:
				helm->focalx = limbs[monster][9][0] - .5;
				helm->focaly = limbs[monster][9][1] - 3.55;
				helm->focalz = limbs[monster][9][2] + 2.5;
				break;
			case GOATMAN:
				helm->focalx = limbs[monster][9][0] - .5;
				helm->focaly = limbs[monster][9][1] - 3.55;
				helm->focalz = limbs[monster][9][2] + 2.75;
				break;
			case INCUBUS:
			case SUCCUBUS:
				helm->focalx = limbs[monster][9][0] - .5;
				helm->focaly = limbs[monster][9][1] - 3.2;
				helm->focalz = limbs[monster][9][2] + 2.5;
				break;
			default:
				break;
		}
		helm->roll = PI / 2;
	}
	else if ( helm->sprite >= items[HAT_HOOD].index && helm->sprite < items[HAT_HOOD].index + items[HAT_HOOD].variations )
	{
		switch ( monster )
		{
			case AUTOMATON:
			case SKELETON:
				helm->focalx = limbs[monster][9][0] - .5;
				helm->focaly = limbs[monster][9][1] - 2.5;
				helm->focalz = limbs[monster][9][2] + 2.25;
				if ( helm->sprite == (items[HAT_HOOD].index + 2) )
				{
					helm->focaly += 0.5; // black hood
				}
				else if ( helm->sprite == (items[HAT_HOOD].index + 3) )
				{
					helm->focaly -= 0.5; // purple hood
				}
				break;
			case INCUBUS:
			case SUCCUBUS:
				helm->focalx = limbs[monster][9][0] - .5;
				helm->focaly = limbs[monster][9][1] - 2.5;
				helm->focalz = limbs[monster][9][2] + 2.5;
				if ( helm->sprite == (items[HAT_HOOD].index + 3) )
				{
					helm->focaly -= 0.5; // purple hood
				}
				break;
			case VAMPIRE:
			case SHOPKEEPER:
			case HUMAN:
				helm->focalx = limbs[monster][9][0] - .5;
				helm->focaly = limbs[monster][9][1] - 2.5;
				helm->focalz = limbs[monster][9][2] + 2.25;
				break;
			case GOATMAN:
				helm->focalx = limbs[monster][9][0] - .5;
				helm->focaly = limbs[monster][9][1] - 2.75;
				helm->focalz = limbs[monster][9][2] + 2.75;
				if ( helm->sprite == (items[HAT_HOOD].index + 2) )
				{
					helm->focaly -= 0.25; // black hood
				}
				else if ( helm->sprite == (items[HAT_HOOD].index + 3) )
				{
					helm->focaly -= 0.5; // purple hood
				}
				break;
			case INSECTOID:
				helm->focalx = limbs[monster][9][0] - .5;
				helm->focaly = limbs[monster][9][1] - 2.15;
				helm->focalz = limbs[monster][9][2] + 2.25;
				if ( helm->sprite == (items[HAT_HOOD].index + 2) )
				{
					helm->focaly += 0.25; // black hood
				}
				else if ( helm->sprite == (items[HAT_HOOD].index + 3) )
				{
					helm->focaly -= 0.5; // purple hood
				}
				break;
			case GOBLIN:
			case SHADOW:
				helm->focalx = limbs[monster][9][0] - .5;
				helm->focaly = limbs[monster][9][1] - 2.75;
				helm->focalz = limbs[monster][9][2] + 2.5;
				if ( monster == GOBLIN && this->sprite == 752 ) // special female offset.
				{
					if ( helm->sprite == (items[HAT_HOOD].index + 3) )
					{
						helm->focaly -= 0.5; // purple hood
					}
				}
				break;
			default:
				break;
		}
		helm->roll = PI / 2;
	}
	else if ( helm->sprite == items[HAT_WIZARD].index || helm->sprite == items[HAT_JESTER].index )
	{
		switch ( monster )
		{
			case AUTOMATON:
			case SKELETON:
				helm->focalx = limbs[monster][9][0];
				helm->focaly = limbs[monster][9][1] - 4.5;
				helm->focalz = limbs[monster][9][2] + 2.25;
				break;
			case INCUBUS:
			case SUCCUBUS:
				helm->focalx = limbs[monster][9][0];
				helm->focaly = limbs[monster][9][1] - 4.75;
				helm->focalz = limbs[monster][9][2] + 2.5;
				break;
			case VAMPIRE:
			case SHOPKEEPER:
			case HUMAN:
				helm->focalx = limbs[monster][9][0];
				helm->focaly = limbs[monster][9][1] - 4.75;
				helm->focalz = limbs[monster][9][2] + 2.25;
				break;
			case GOATMAN:
				helm->focalx = limbs[monster][9][0];
				helm->focaly = limbs[monster][9][1] - 5.f;
				helm->focalz = limbs[monster][9][2] + 2.75;
				break;
			case INSECTOID:
				helm->focalx = limbs[monster][9][0];
				helm->focaly = limbs[monster][9][1] - 4.75;
				helm->focalz = limbs[monster][9][2] + 2.25;
				break;
			case GOBLIN:
			case SHADOW:
				helm->focalx = limbs[monster][9][0];
				helm->focaly = limbs[monster][9][1] - 5;
				helm->focalz = limbs[monster][9][2] + 2.5;
				break;
			default:
				break;
		}
		helm->roll = PI / 2;
	}
	else if ( helm->sprite == items[HAT_FEZ].index )
	{
		switch ( monster )
		{
			case AUTOMATON:
			case SKELETON:
				helm->focalx = limbs[monster][9][0];
				helm->focaly = limbs[monster][9][1] - 4.f;
				helm->focalz = limbs[monster][9][2] + 2.25;
				break;
			case INCUBUS:
			case SUCCUBUS:
				helm->focalx = limbs[monster][9][0];
				helm->focaly = limbs[monster][9][1] - 4.0;
				helm->focalz = limbs[monster][9][2] + 2.5;
				break;
			case VAMPIRE:
			case SHOPKEEPER:
			case HUMAN:
				helm->focalx = limbs[monster][9][0];
				helm->focaly = limbs[monster][9][1] - 4.35;
				helm->focalz = limbs[monster][9][2] + 2.25;
				break;
			case GOATMAN:
				helm->focalx = limbs[monster][9][0];
				helm->focaly = limbs[monster][9][1] - 4.5;
				helm->focalz = limbs[monster][9][2] + 2.75;
				break;
			case INSECTOID:
				helm->focalx = limbs[monster][9][0];
				helm->focaly = limbs[monster][9][1] - 4;
				helm->focalz = limbs[monster][9][2] + 2.25;
				break;
			case GOBLIN:
			case SHADOW:
				helm->focalx = limbs[monster][9][0];
				helm->focaly = limbs[monster][9][1] - 4.5;
				helm->focalz = limbs[monster][9][2] + 2.5;
				if ( monster == GOBLIN && this->sprite == 752 ) // special female offset.
				{
					helm->focaly -= 0.25;
				}
				break;
			default:
				break;
		}
		helm->roll = PI / 2;
	}
	else
	{
		if ( monster == GOBLIN && this->sprite == 752 ) // special female offset.
		{
			helm->focalz = limbs[monster][9][2] - 0.25; // all non-hat helms
		}
	}
}

real_t Entity::yawDifferenceFromPlayer(int player)
{
	if ( player >= 0 && players[player] && players[player]->entity )
	{
		real_t targetYaw = this->yaw;
		while ( targetYaw >= 2 * PI )
		{
			targetYaw -= PI * 2;
		}
		while ( targetYaw < 0 )
		{
			targetYaw += PI * 2;
		}
		return (PI - abs(abs(players[player]->entity->yaw - targetYaw) - PI)) * 2;
	}
	return 0.f;
}

Entity* summonChest(long x, long y)
{
	Entity* entity = newEntity(21, 1, map.entities, nullptr); //Chest entity.
	if ( !entity )
	{
		return nullptr;
	}
	entity->chestLocked = -1;

	// Find a free tile next to the source and then spawn it there.
	if ( multiplayer != CLIENT )
	{
		if ( entityInsideSomething(entity) )
		{
			do
			{
				entity->x = x;
				entity->y = y - 16;
				if (!entityInsideSomething(entity))
				{
					break;    // north
				}
				entity->x = x;
				entity->y = y + 16;
				if (!entityInsideSomething(entity))
				{
					break;    // south
				}
				entity->x = x - 16;
				entity->y = y;
				if (!entityInsideSomething(entity))
				{
					break;    // west
				}
				entity->x = x + 16;
				entity->y = y;
				if (!entityInsideSomething(entity))
				{
					break;    // east
				}
				entity->x = x + 16;
				entity->y = y - 16;
				if (!entityInsideSomething(entity))
				{
					break;    // northeast
				}
				entity->x = x + 16;
				entity->y = y + 16;
				if (!entityInsideSomething(entity))
				{
					break;    // southeast
				}
				entity->x = x - 16;
				entity->y = y - 16;
				if (!entityInsideSomething(entity))
				{
					break;    // northwest
				}
				entity->x = x - 16;
				entity->y = y + 16;
				if (!entityInsideSomething(entity))
				{
					break;    // southwest
				}

				// we can't have monsters in walls...
				list_RemoveNode(entity->mynode);
				entity = nullptr;
				break;
			}
			while (1);
		}
	}

	entity->sizex = 3;
	entity->sizey = 2;
	entity->x = x;
	entity->y = y;
	entity->x += 8;
	entity->y += 8;
	entity->z = 5.5;
	entity->yaw = entity->yaw * (PI / 2); //set to 0 by default in editor, can be set 0-3
	entity->behavior = &actChest;
	entity->sprite = 188;
	//entity->skill[9] = -1; //Set default chest as random category < 0

	Entity* childEntity = newEntity(216, 0, map.entities, nullptr); //Sort-of limb entity.
	if ( !childEntity )
	{
		return nullptr;
	}
	childEntity->parent = entity->getUID();
	entity->parent = childEntity->getUID();
	if ( entity->yaw == 0 ) //EAST FACING
	{
		childEntity->x = entity->x - 3;
		childEntity->y = entity->y;
	}
	else if ( entity->yaw == PI / 2 ) //SOUTH FACING
	{
		childEntity->x = entity->x;
		childEntity->y = entity->y - 3;
	}
	else if ( entity->yaw == PI ) //WEST FACING
	{
		childEntity->x = entity->x + 3;
		childEntity->y = entity->y;
	}
	else if (entity->yaw == 3 * PI/2 ) //NORTH FACING
	{
		childEntity->x = entity->x;
		childEntity->y = entity->y + 3;
	}
	else
	{
		childEntity->x = entity->x;
		childEntity->y = entity->y - 3;
	}
	//printlog("29 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);
	childEntity->z = entity->z - 2.75;
	childEntity->focalx = 3;
	childEntity->focalz = -.75;
	childEntity->yaw = entity->yaw;
	childEntity->sizex = 2;
	childEntity->sizey = 2;
	childEntity->behavior = &actChestLid;
	childEntity->flags[PASSABLE] = true;

	//Chest inventory.
	node_t* tempNode = list_AddNodeFirst(&entity->children);
	tempNode->element = nullptr;
	tempNode->deconstructor = &emptyDeconstructor;

	return entity;
}

void Entity::addToCreatureList(list_t *list)
{
	//printlog("*ATTEMPTING* to add Dennis to creature list.");
	if ( list )
	{
		if ( myCreatureListNode )
		{
			list_RemoveNode(myCreatureListNode);
			myCreatureListNode = nullptr;
		}
		myCreatureListNode = list_AddNodeLast(list);
		myCreatureListNode->element = this;
		myCreatureListNode->deconstructor = &emptyDeconstructor;
		myCreatureListNode->size = sizeof(Entity);
		//printlog("Added dennis to creature list.");
	}
}

int Entity::getMagicResistance()
{
	int resistance = 0;
	Stat* myStats = getStats();
	if ( myStats )
	{
		if ( myStats->shield )
		{
			if ( myStats->shield->type == STEEL_SHIELD_RESISTANCE )
			{
				if ( myStats->defending )
				{
					resistance += 2;
				}
				else
				{
					resistance += 1;
				}
			}
		}
		if ( myStats->ring )
		{
			if ( myStats->ring->type == RING_MAGICRESISTANCE )
			{
				resistance += 1;
			}
		}
		if ( myStats->gloves )
		{
			if ( myStats->gloves->type == ARTIFACT_GLOVES )
			{
				resistance += 1;
			}
		}
		if ( myStats->EFFECTS[EFF_MAGICRESIST] )
		{
			resistance += 1;
		}
		if ( myStats->EFFECTS[EFF_SHRINE_BLUE_BUFF] )
		{
			resistance += 1;
		}
	}
	else
	{
		return 0;
	}
	return resistance;
}

void Entity::setHardcoreStats(Stat& stats)
{
	if ( (svFlags & SV_FLAG_HARDCORE) )
	{
		// spice up some stats...
		int statIncrease = ((abs(stats.HP) / 20 + 1) * 20); // each 20 HP add 20 random HP
		stats.HP += statIncrease - (rand() % (std::max(statIncrease / 5, 1))); // 80%-100% of increased value
		stats.MAXHP = stats.HP;
		stats.OLDHP = stats.HP;

		statIncrease = (abs(stats.STR) / 5 + 1) * 5; // each 5 STR add 5 more STR.
		stats.STR += (statIncrease - (rand() % (std::max(statIncrease / 4, 1)))); // 75%-100% of increased value.

		statIncrease = (abs(stats.PER) / 5 + 1) * 5; // each 5 PER add 5 more PER.
		stats.PER += (statIncrease - (rand() % (std::max(statIncrease / 4, 1)))); // 75%-100% of increased value.

		statIncrease = std::min((abs(stats.DEX) / 4 + 1) * 1, 8); // each 4 DEX add 1 more DEX, capped at 8.
		stats.DEX += (statIncrease - (rand() % (std::max(statIncrease / 2, 1)))); // 50%-100% of increased value.

		statIncrease = (abs(stats.CON) / 5 + 1) * 1; // each 5 CON add 1 more CON.
		stats.CON += (statIncrease - (rand() % (std::max(statIncrease / 2, 1)))); // 50%-100% of increased value.

		statIncrease = (abs(stats.INT) / 5 + 1) * 5; // each 5 INT add 5 more INT.
		stats.INT += (statIncrease - (rand() % (std::max(statIncrease / 2, 1)))); // 50%-100% of increased value.

		int lvlIncrease = rand() % 4;
		lvlIncrease = std::max(0, lvlIncrease - 1);
		stats.LVL += (lvlIncrease - 1); // increase by 1 or 2 50%, else stay same.
	}
	//messagePlayer(0, "Set stats to: ");
	//messagePlayer(0, "MAXHP: %d", stats.MAXHP);
	//messagePlayer(0, "HP: %d", stats.HP);
	//messagePlayer(0, "MAXMP: %d", stats.MAXMP);
	//messagePlayer(0, "MP: %d", stats.MP);
	//messagePlayer(0, "Str: %d", stats.STR);
	//messagePlayer(0, "Dex: %d", stats.DEX);
	//messagePlayer(0, "Con: %d", stats.CON);
	//messagePlayer(0, "Int: %d", stats.INT);
	//messagePlayer(0, "Per: %d", stats.PER);
	//messagePlayer(0, "Chr: %d", stats.CHR);
	//messagePlayer(0, "LVL: %d", stats.LVL);
	//messagePlayer(0, "GOLD: %d", stats.GOLD);
}

int playerEntityMatchesUid(Uint32 uid)
{
	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		if ( players[i] && players[i]->entity && players[i]->entity->getUID() == uid )
		{
			return i;
		}
	}

	return -1;
}

bool monsterNameIsGeneric(Stat& monsterStats)
{
	if ( strstr(monsterStats.name, "lesser")
		|| strstr(monsterStats.name, "young")
		|| strstr(monsterStats.name, "enslaved")
		|| strstr(monsterStats.name, "damaged")
		|| strstr(monsterStats.name, "corrupted")
		|| strstr(monsterStats.name, "cultist")
		|| strstr(monsterStats.name, "knight")
		|| strstr(monsterStats.name, "sentinel")
		|| strstr(monsterStats.name, "mage") )
	{
		// If true, pretend the monster doesn't have a name and use the generic message "You hit the lesser skeleton!"
		return true;
	}
	return false;
}

node_t* TileEntityListHandler::addEntity(Entity& entity)
{
	if ( entity.myTileListNode )
	{
		return nullptr;
	}

	if ( entity.getUID() == -3 )
	{
		return nullptr;
	}

	int x = (static_cast<int>(entity.x) >> 4);
	int y = (static_cast<int>(entity.y) >> 4);
	if ( x >= 0 && x < kMaxMapDimension && y >= 0 && y < kMaxMapDimension )
	{
		//messagePlayer(0, "added at %d, %d", x, y);
		entity.myTileListNode = list_AddNodeLast(&TileEntityList.gridEntities[x][y]);
		entity.myTileListNode->element = &entity;
		entity.myTileListNode->deconstructor = &emptyDeconstructor;
		entity.myTileListNode->size = sizeof(Entity);
		return entity.myTileListNode;
	}

	return nullptr;
}

node_t* TileEntityListHandler::updateEntity(Entity& entity)
{
	if ( !entity.myTileListNode )
	{
		return nullptr;
	}

	int x = (static_cast<int>(entity.x) >> 4);
	int y = (static_cast<int>(entity.y) >> 4);
	if ( x >= 0 && x < kMaxMapDimension && y >= 0 && y < kMaxMapDimension )
	{
		list_RemoveNode(entity.myTileListNode);
		entity.myTileListNode = list_AddNodeLast(&TileEntityList.gridEntities[x][y]);
		entity.myTileListNode->element = &entity;
		entity.myTileListNode->deconstructor = &emptyDeconstructor;
		entity.myTileListNode->size = sizeof(Entity);
		return entity.myTileListNode;
	}

	return nullptr;
}

void TileEntityListHandler::clearTile(int x, int y)
{
	list_FreeAll(&gridEntities[x][y]);
}

void TileEntityListHandler::emptyGridEntities()
{
	for ( int i = 0; i < kMaxMapDimension; ++i )
	{
		for ( int j = 0; j < kMaxMapDimension; ++j )
		{
			clearTile(i, j);
		}
	}
}

list_t* TileEntityListHandler::getTileList(int x, int y)
{
	if ( x >= 0 && x < kMaxMapDimension && y >= 0 && y < kMaxMapDimension )
	{
		return &gridEntities[x][y];
	}
	return nullptr;
}

/* returns list of entities within a radius, e.g 1 radius is a 3x3 area around given center. */
std::vector<list_t*> TileEntityListHandler::getEntitiesWithinRadius(int u, int v, int radius)
{
	std::vector<list_t*> return_val;

	for ( int i = u - radius; i <= u + radius; ++i )
	{
		for ( int j = v - radius; j <= v + radius; ++j )
		{
			list_t* list = getTileList(i, j);
			if ( list )
			{
				return_val.push_back(list);
			}
		}
	}

	return return_val;
}

/* returns list of entities within a radius around entity, e.g 1 radius is a 3x3 area around entity. */
std::vector<list_t*> TileEntityListHandler::getEntitiesWithinRadiusAroundEntity(Entity* entity, int radius)
{
	int u = static_cast<int>(entity->x) >> 4;
	int v = static_cast<int>(entity->y) >> 4;
	return getEntitiesWithinRadius(u, v, radius);
}

void Entity::setHumanoidLimbOffset(Entity* limb, Monster race, int limbType)
{
	if ( !limb )
	{
		return;
	}
	switch ( race )
	{
		case HUMAN:
		case VAMPIRE:
			if ( limbType == LIMB_HUMANOID_TORSO )
			{
				limb->x -= .25 * cos(this->yaw);
				limb->y -= .25 * sin(this->yaw);
				limb->z += 2.5;
			}
			else if ( limbType == LIMB_HUMANOID_RIGHTLEG )
			{
				limb->x += 1 * cos(this->yaw + PI / 2) + .25 * cos(this->yaw);
				limb->y += 1 * sin(this->yaw + PI / 2) + .25 * sin(this->yaw);
				limb->z += 5;
				if ( this->z >= 1.4 && this->z <= 1.6 )
				{
					limb->yaw += PI / 8;
					limb->pitch = -PI / 2;
				}
			}
			else if ( limbType == LIMB_HUMANOID_LEFTLEG )
			{
				limb->x -= 1 * cos(this->yaw + PI / 2) - .25 * cos(this->yaw);
				limb->y -= 1 * sin(this->yaw + PI / 2) - .25 * sin(this->yaw);
				limb->z += 5;
				if ( this->z >= 1.4 && this->z <= 1.6 )
				{
					limb->yaw -= PI / 8;
					limb->pitch = -PI / 2;
				}
			}
			else if ( limbType == LIMB_HUMANOID_RIGHTARM )
			{
				limb->x += 2.5 * cos(this->yaw + PI / 2) - .20 * cos(this->yaw);
				limb->y += 2.5 * sin(this->yaw + PI / 2) - .20 * sin(this->yaw);
				limb->z += 1.5;
				if ( this->z >= 1.4 && this->z <= 1.6 )
				{
					limb->pitch = 0;
				}
			}
			else if ( limbType == LIMB_HUMANOID_LEFTARM )
			{
				limb->x -= 2.5 * cos(this->yaw + PI / 2) + .20 * cos(this->yaw);
				limb->y -= 2.5 * sin(this->yaw + PI / 2) + .20 * sin(this->yaw);
				limb->z += 1.5;
				if ( this->z >= 1.4 && this->z <= 1.6 )
				{
					limb->pitch = 0;
				}
			}
			break;
		case SKELETON:
		case AUTOMATON:
			if ( limbType == LIMB_HUMANOID_TORSO )
			{
				limb->x -= .25 * cos(this->yaw);
				limb->y -= .25 * sin(this->yaw);
				limb->z += 2;
				if ( limb->sprite == items[WIZARD_DOUBLET].index
					|| limb->sprite == items[HEALER_DOUBLET].index
					|| limb->sprite == items[TUNIC].index
					|| limb->sprite == items[TUNIC].index + 1 )
				{
					limb->z += 0.15;
					limb->scalez = 0.9;
				}
				else
				{
					limb->scalez = 1.f;
				}
			}
			else if ( limbType == LIMB_HUMANOID_RIGHTLEG )
			{
				limb->x += 1 * cos(this->yaw + PI / 2) + .25 * cos(this->yaw);
				limb->y += 1 * sin(this->yaw + PI / 2) + .25 * sin(this->yaw);
				limb->z += 4;
				if ( this->z >= 1.9 && this->z <= 2.1 )
				{
					limb->yaw += PI / 8;
					limb->pitch = -PI / 2;
				}
				else if ( limb->pitch <= -PI / 3 )
				{
					limb->pitch = 0;
				}
			}
			else if ( limbType == LIMB_HUMANOID_LEFTLEG )
			{
				limb->x -= 1 * cos(this->yaw + PI / 2) - .25 * cos(this->yaw);
				limb->y -= 1 * sin(this->yaw + PI / 2) - .25 * sin(this->yaw);
				limb->z += 4;
				if ( this->z >= 1.9 && this->z <= 2.1 )
				{
					limb->yaw -= PI / 8;
					limb->pitch = -PI / 2;
				}
				else if ( limb->pitch <= -PI / 3 )
				{
					limb->pitch = 0;
				}
			}
			else if ( limbType == LIMB_HUMANOID_RIGHTARM )
			{
				if ( limb->sprite != 689 && limb->sprite != 691
					&& limb->sprite != 233 && limb->sprite != 234
					&& limb->sprite != 745 && limb->sprite != 747
					&& limb->sprite != 471 && limb->sprite != 472 )
				{
					// wearing gloves (not default arms), position tighter to body.
					limb->x += 1.75 * cos(this->yaw + PI / 2) - .20 * cos(this->yaw);
					limb->y += 1.75 * sin(this->yaw + PI / 2) - .20 * sin(this->yaw);
				}
				else
				{
					limb->x += 2.f * cos(this->yaw + PI / 2) - .20 * cos(this->yaw);
					limb->y += 2.f * sin(this->yaw + PI / 2) - .20 * sin(this->yaw);
				}
				limb->z += .6;
				if ( this->z >= 1.9 && this->z <= 2.1 )
				{
					limb->pitch = 0;
				}
			}
			else if ( limbType == LIMB_HUMANOID_LEFTARM )
			{
				if ( limb->sprite != 688 && limb->sprite != 690
					&& limb->sprite != 231 && limb->sprite != 232
					&& limb->sprite != 744 && limb->sprite != 746
					&& limb->sprite != 469 && limb->sprite != 470 )
				{
					// wearing gloves (not default arms), position tighter to body.
					limb->x -= 1.75 * cos(this->yaw + PI / 2) + .20 * cos(this->yaw);
					limb->y -= 1.75 * sin(this->yaw + PI / 2) + .20 * sin(this->yaw);
				}
				else
				{
					limb->x -= 2.f * cos(this->yaw + PI / 2) + .20 * cos(this->yaw);
					limb->y -= 2.f * sin(this->yaw + PI / 2) + .20 * sin(this->yaw);
				}
				limb->z += .6;
				if ( this->z >= 1.9 && this->z <= 2.1 )
				{
					limb->pitch = 0;
				}
			}
			break;
		case GOBLIN:
		case GOATMAN:
		case INSECTOID:
			if ( limbType == LIMB_HUMANOID_TORSO )
			{
				limb->x -= .25 * cos(this->yaw);
				limb->y -= .25 * sin(this->yaw);
				limb->z += 2;
				if ( race == INSECTOID )
				{
					if ( limb->sprite != 727 && limb->sprite != 458 && limb->sprite != 761 )
					{
						// wearing armor, offset by 1.
						limb->z -= 1;
					}
				}
			}
			else if ( limbType == LIMB_HUMANOID_RIGHTLEG )
			{
				limb->x += 1 * cos(this->yaw + PI / 2) + .25 * cos(this->yaw);
				limb->y += 1 * sin(this->yaw + PI / 2) + .25 * sin(this->yaw);
				limb->z += 4;
				if ( this->z >= 2.4 && this->z <= 2.6 )
				{
					limb->yaw += PI / 8;
					limb->pitch = -PI / 2;
				}
				else if ( limb->pitch <= -PI / 3 )
				{
					limb->pitch = 0;
				}
			}
			else if ( limbType == LIMB_HUMANOID_LEFTLEG )
			{
				limb->x -= 1 * cos(this->yaw + PI / 2) - .25 * cos(this->yaw);
				limb->y -= 1 * sin(this->yaw + PI / 2) - .25 * sin(this->yaw);
				limb->z += 4;
				if ( this->z >= 2.4 && this->z <= 2.6 )
				{
					limb->yaw -= PI / 8;
					limb->pitch = -PI / 2;
				}
				else if ( limb->pitch <= -PI / 3 )
				{
					limb->pitch = 0;
				}
			}
			else if ( limbType == LIMB_HUMANOID_RIGHTARM )
			{
				limb->x += 2.5 * cos(this->yaw + PI / 2) - .20 * cos(this->yaw);
				limb->y += 2.5 * sin(this->yaw + PI / 2) - .20 * sin(this->yaw);
				limb->z += .5;
				if ( this->z >= 2.4 && this->z <= 2.6 )
				{
					limb->pitch = 0;
				}
			}
			else if ( limbType == LIMB_HUMANOID_LEFTARM )
			{
				limb->x -= 2.5 * cos(this->yaw + PI / 2) + .20 * cos(this->yaw);
				limb->y -= 2.5 * sin(this->yaw + PI / 2) + .20 * sin(this->yaw);
				limb->z += .5;
				if ( this->z >= 2.4 && this->z <= 2.6 )
				{
					limb->pitch = 0;
				}
			}
			break;
		case INCUBUS:
		case SUCCUBUS:
			if ( limbType == LIMB_HUMANOID_TORSO )
			{
				limb->x -= .5 * cos(this->yaw);
				limb->y -= .5 * sin(this->yaw);
				limb->z += 2.5;
			}
			else if ( limbType == LIMB_HUMANOID_RIGHTLEG )
			{
				limb->x += 1 * cos(this->yaw + PI / 2) - .75 * cos(this->yaw);
				limb->y += 1 * sin(this->yaw + PI / 2) - .75 * sin(this->yaw);
				limb->z += 5;
				if ( this->z >= 1.4 && this->z <= 1.6 )
				{
					limb->yaw += PI / 8;
					limb->pitch = -PI / 2;
				}
				else if ( limb->pitch <= -PI / 3 )
				{
					limb->pitch = 0;
				}
			}
			else if ( limbType == LIMB_HUMANOID_LEFTLEG )
			{
				limb->x -= 1 * cos(this->yaw + PI / 2) + .75 * cos(this->yaw);
				limb->y -= 1 * sin(this->yaw + PI / 2) + .75 * sin(this->yaw);
				limb->z += 5;
				if ( this->z >= 1.4 && this->z <= 1.6 )
				{
					limb->yaw -= PI / 8;
					limb->pitch = -PI / 2;
				}
				else if ( limb->pitch <= -PI / 3 )
				{
					limb->pitch = 0;
				}
			}
			else if ( limbType == LIMB_HUMANOID_RIGHTARM )
			{
				limb->x += 2.5 * cos(this->yaw + PI / 2) - .20 * cos(this->yaw);
				limb->y += 2.5 * sin(this->yaw + PI / 2) - .20 * sin(this->yaw);
				limb->z += .5;
				if ( this->z >= 1.4 && this->z <= 1.6 )
				{
					limb->pitch = 0;
				}
			}
			else if ( limbType == LIMB_HUMANOID_LEFTARM )
			{
				limb->x -= 2.5 * cos(this->yaw + PI / 2) + .20 * cos(this->yaw);
				limb->y -= 2.5 * sin(this->yaw + PI / 2) + .20 * sin(this->yaw);
				limb->z += .5;
				if ( this->z >= 1.4 && this->z <= 1.6 )
				{
					limb->pitch = 0;
				}
			}
			break;
		default:
			break;
	}
}

void Entity::handleHumanoidShieldLimb(Entity* shieldLimb, Entity* shieldArmLimb)
{
	if ( !shieldLimb || !shieldArmLimb )
	{
		return;
	}

	int race = this->getMonsterTypeFromSprite();
	int player = -1;
	if ( this->behavior == &actPlayer )
	{
		player = this->skill[2];
	}
	Entity* flameEntity = nullptr;

	shieldLimb->focalx = limbs[race][7][0];
	shieldLimb->focaly = limbs[race][7][1];
	shieldLimb->focalz = limbs[race][7][2];

	switch ( race )
	{
		case HUMAN:
		case VAMPIRE:
			shieldLimb->x -= 2.5 * cos(this->yaw + PI / 2) + .20 * cos(this->yaw);
			shieldLimb->y -= 2.5 * sin(this->yaw + PI / 2) + .20 * sin(this->yaw);
			shieldLimb->z += 2.5;
			shieldLimb->yaw = shieldArmLimb->yaw;
			shieldLimb->roll = 0;
			shieldLimb->pitch = 0;

			if ( shieldLimb->sprite == items[TOOL_TORCH].index )
			{
				flameEntity = spawnFlame(shieldLimb, SPRITE_FLAME);
				flameEntity->x += 2 * cos(shieldArmLimb->yaw);
				flameEntity->y += 2 * sin(shieldArmLimb->yaw);
				flameEntity->z -= 2;
			}
			else if ( shieldLimb->sprite == items[TOOL_CRYSTALSHARD].index )
			{
				flameEntity = spawnFlame(shieldLimb, SPRITE_CRYSTALFLAME);
				flameEntity->x += 2 * cos(shieldArmLimb->yaw);
				flameEntity->y += 2 * sin(shieldArmLimb->yaw);
				flameEntity->z -= 2;
			}
			else if ( shieldLimb->sprite == items[TOOL_LANTERN].index )
			{
				shieldLimb->z += 2;
				flameEntity = spawnFlame(shieldLimb, SPRITE_FLAME);
				flameEntity->x += 2 * cos(shieldArmLimb->yaw);
				flameEntity->y += 2 * sin(shieldArmLimb->yaw);
				flameEntity->z += 1;
			}

			if ( this->fskill[8] > PI / 32 ) //MONSTER_SHIELDYAW and PLAYER_SHIELDYAW defending animation
			{
				if ( shieldLimb->sprite != items[TOOL_TORCH].index 
					&& shieldLimb->sprite != items[TOOL_LANTERN].index
					&& shieldLimb->sprite != items[TOOL_CRYSTALSHARD].index )
				{
					// shield, so rotate a little.
					shieldLimb->roll += PI / 64;
				}
				else
				{
					shieldLimb->x += 0.25 * cos(this->yaw);
					shieldLimb->y += 0.25 * sin(this->yaw);
					shieldLimb->pitch += PI / 16;
					if ( flameEntity )
					{
						flameEntity->x += 0.75 * cos(shieldArmLimb->yaw);
						flameEntity->y += 0.75 * sin(shieldArmLimb->yaw);
					}
				}
			}
			break;
		case SKELETON:
		case AUTOMATON:
			shieldLimb->x -= 3.f * cos(this->yaw + PI / 2) + .20 * cos(this->yaw);
			shieldLimb->y -= 3.f * sin(this->yaw + PI / 2) + .20 * sin(this->yaw);
			shieldLimb->z += 2.5;
			shieldLimb->yaw = shieldArmLimb->yaw;
			shieldLimb->roll = 0;
			shieldLimb->pitch = 0;

			if ( shieldLimb->sprite != items[TOOL_TORCH].index && shieldLimb->sprite != items[TOOL_LANTERN].index && shieldLimb->sprite != items[TOOL_CRYSTALSHARD].index )
			{
				shieldLimb->focalx = limbs[race][7][0] - 0.65;
				shieldLimb->focaly = limbs[race][7][1];
				shieldLimb->focalz = limbs[race][7][2];
			}

			if ( shieldLimb->sprite == items[TOOL_TORCH].index )
			{
				flameEntity = spawnFlame(shieldLimb, SPRITE_FLAME);
				flameEntity->x += 2.5 * cos(shieldLimb->yaw + PI / 16);
				flameEntity->y += 2.5 * sin(shieldLimb->yaw + PI / 16);
				flameEntity->z -= 2;
			}
			else if ( shieldLimb->sprite == items[TOOL_CRYSTALSHARD].index )
			{
				flameEntity = spawnFlame(shieldLimb, SPRITE_CRYSTALFLAME);
				flameEntity->x += 2.5 * cos(shieldLimb->yaw + PI / 16);
				flameEntity->y += 2.5 * sin(shieldLimb->yaw + PI / 16);
				flameEntity->z -= 2;
			}
			else if ( shieldLimb->sprite == items[TOOL_LANTERN].index )
			{
				shieldLimb->z += 2;
				flameEntity = spawnFlame(shieldLimb, SPRITE_FLAME);
				flameEntity->x += 2.5 * cos(shieldLimb->yaw);
				flameEntity->y += 2.5 * sin(shieldLimb->yaw);
				flameEntity->z += 1;
			}

			if ( this->fskill[8] > PI / 32 ) //MONSTER_SHIELDYAW and PLAYER_SHIELDYAW defending animation
			{
				if ( shieldLimb->sprite != items[TOOL_TORCH].index
					&& shieldLimb->sprite != items[TOOL_LANTERN].index
					&& shieldLimb->sprite != items[TOOL_CRYSTALSHARD].index )
				{
					// shield, so rotate a little.
					shieldLimb->roll += PI / 64;
				}
				else
				{
					shieldLimb->x += 0.25 * cos(this->yaw);
					shieldLimb->y += 0.25 * sin(this->yaw);
					shieldLimb->pitch += PI / 16;
					if ( flameEntity )
					{
						flameEntity->x += 0.75 * cos(shieldArmLimb->yaw);
						flameEntity->y += 0.75 * sin(shieldArmLimb->yaw);
					}
				}
			}
			else
			{
				if ( !shieldLimb->flags[INVISIBLE] )
				{
					shieldArmLimb->yaw -= PI / 8;
				}
			}
			break;
		case GOBLIN:
		case GOATMAN:
		case INSECTOID:
		case INCUBUS:
		case SUCCUBUS:
			shieldLimb->x -= 2.5 * cos(this->yaw + PI / 2) + .20 * cos(this->yaw);
			shieldLimb->y -= 2.5 * sin(this->yaw + PI / 2) + .20 * sin(this->yaw);
			shieldLimb->z += 2.5;
			shieldLimb->yaw = shieldArmLimb->yaw;
			shieldLimb->roll = 0;
			shieldLimb->pitch = 0;

			/*if ( shieldLimb->sprite != items[TOOL_TORCH].index && shieldLimb->sprite != items[TOOL_LANTERN].index && shieldLimb->sprite != items[TOOL_CRYSTALSHARD].index )
			{
				shieldLimb->focalx = limbs[race][7][0];
				shieldLimb->focaly = limbs[race][7][1];
				shieldLimb->focalz = limbs[race][7][2];
			}
			else
			{
				shieldLimb->focalx = limbs[race][7][0] - 0.5;
				shieldLimb->focaly = limbs[race][7][1] - 1;
				shieldLimb->focalz = limbs[race][7][2];
			}*/

			if ( shieldLimb->sprite == items[TOOL_TORCH].index )
			{
				flameEntity = spawnFlame(shieldLimb, SPRITE_FLAME);
				flameEntity->x += 2 * cos(shieldLimb->yaw);
				flameEntity->y += 2 * sin(shieldLimb->yaw);
				flameEntity->z -= 2;
			}
			else if ( shieldLimb->sprite == items[TOOL_CRYSTALSHARD].index )
			{
				flameEntity = spawnFlame(shieldLimb, SPRITE_CRYSTALFLAME);
				flameEntity->x += 2 * cos(shieldLimb->yaw);
				flameEntity->y += 2 * sin(shieldLimb->yaw);
				flameEntity->z -= 2;
			}
			else if ( shieldLimb->sprite == items[TOOL_LANTERN].index )
			{
				shieldLimb->z += 2;
				flameEntity = spawnFlame(shieldLimb, SPRITE_FLAME);
				flameEntity->x += 2 * cos(shieldLimb->yaw);
				flameEntity->y += 2 * sin(shieldLimb->yaw);
				flameEntity->z += 1;
			}

			if ( this->fskill[8] > PI / 32 ) //MONSTER_SHIELDYAW and PLAYER_SHIELDYAW defending animation
			{
				if ( shieldLimb->sprite != items[TOOL_TORCH].index
					&& shieldLimb->sprite != items[TOOL_LANTERN].index
					&& shieldLimb->sprite != items[TOOL_CRYSTALSHARD].index )
				{
					// shield, so rotate a little.
					shieldLimb->roll += PI / 64;
				}
				else
				{
					shieldLimb->x += 0.25 * cos(this->yaw);
					shieldLimb->y += 0.25 * sin(this->yaw);
					shieldLimb->pitch += PI / 16;
					if ( flameEntity )
					{
						flameEntity->x += 0.75 * cos(shieldArmLimb->yaw);
						flameEntity->y += 0.75 * sin(shieldArmLimb->yaw);
					}
				}
			}
			break;
		default:
			break;
	}

	if ( flameEntity && player >= 0 )
	{
		if ( player == clientnum )
		{
			flameEntity->flags[GENIUS] = true;
			flameEntity->setUID(-4);
		}
		else
		{
			flameEntity->setUID(-3);
		}
	}
}