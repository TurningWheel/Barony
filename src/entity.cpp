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
#include "engine/audio/sound.hpp"
#include "magic/magic.hpp"
#include "interface/interface.hpp"
#include "net.hpp"
#include "collision.hpp"
#include "paths.hpp"
#include "book.hpp"
#include "shops.hpp"
#ifdef STEAMWORKS
#include <steam/steam_api.h>
#endif
#include "player.hpp"
#include "scores.hpp"
#include "menu.hpp"
#include "mod_tools.hpp"
#include "interface/ui.hpp"
#ifdef __ARM_NEON__
#include <arm_neon.h>
#endif
#include "ui/MainMenu.hpp"
#include "ui/GameUI.hpp"

/*-------------------------------------------------------------------------------

Entity::Entity)

Construct an Entity

-------------------------------------------------------------------------------*/

ConsoleVariable<int> cvar_entity_bodypart_sync_tick("/entity_bodypart_sync_tick", TICKS_PER_SECOND / 4);
Entity::Entity(Sint32 in_sprite, Uint32 pos, list_t* entlist, list_t* creaturelist) :
	lightBonus(0.f),
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
	chestHasVampireBook(skill[11]),
	chestLockpickHealth(skill[12]),
	chestOldHealth(skill[15]),
	chestMimicChance(skill[16]),
	char_gonnavomit(skill[26]),
	char_heal(skill[22]),
	char_energize(skill[23]),
	char_drunk(skill[24]),
	char_torchtime(skill[25]),
	char_poison(skill[21]),
	char_fire(skill[36]),
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
	playerVampireCurse(skill[51]),
	playerAutomatonDeathCounter(skill[15]),
	playerCreatedDeathCam(skill[16]),
	monsterAttack(skill[8]),
	monsterAttackTime(skill[9]),
	monsterArmbended(skill[10]),
	monsterWeaponYaw(fskill[5]),
	monsterShadowInitialMimic(skill[34]),
	monsterShadowDontChangeName(skill[35]),
	monsterSlimeLastAttack(skill[34]),
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
	monsterDevilNumSummons(skill[18]),
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
	monsterKnockbackVelocity(fskill[9]),
	monsterKnockbackUID(skill[51]),
	creatureWebbedSlowCount(skill[52]),
	monsterFearfulOfUid(skill[53]),
	creatureShadowTaggedThisUid(skill[54]),
	monsterIllusionTauntingThisUid(skill[55]),
	monsterLastDistractedByNoisemaker(skill[55]), // shares with above as above only applies to inner demons.
	monsterExtraReflexTick(skill[56]),
	monsterSentrybotLookDir(fskill[10]),
	monsterKnockbackTangentDir(fskill[11]),
	playerStrafeVelocity(fskill[12]),
	playerStrafeDir(fskill[13]),
	monsterSpecialAttackUnequipSafeguard(fskill[14]),
	particleDuration(skill[0]),
	particleShrink(skill[1]),
	monsterHitTime(skill[7]),
	itemNotMoving(skill[18]),
	itemNotMovingClient(skill[19]),
	itemSokobanReward(skill[20]),
	itemOriginalOwner(skill[21]),
	itemStolen(skill[22]),
	itemShowOnMap(skill[23]),
	itemDelayMonsterPickingUp(skill[24]),
	itemReceivedDetailsFromServer(skill[25]),
	itemAutoSalvageByPlayer(skill[26]),
	itemSplooshed(skill[27]),
	itemContainer(skill[29]),
	itemWaterBob(fskill[2]),
	gateInit(skill[1]),
	gateStatus(skill[3]),
	gateRattle(skill[4]),
	gateStartHeight(fskill[0]),
	gateVelZ(vel_z),
	gateInverted(skill[5]),
	gateDisableOpening(skill[6]),
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
	doorPreventLockpickExploit(skill[10]),
	doorForceLockedUnlocked(skill[11]),
	doorDisableLockpicks(skill[12]),
	doorDisableOpening(skill[13]),
	doorLockpickHealth(skill[14]),
	doorOldHealth(skill[15]),
	particleTimerDuration(skill[0]),
	particleTimerEndAction(skill[1]),
	particleTimerEndSprite(skill[3]),
	particleTimerCountdownAction(skill[4]),
	particleTimerCountdownSprite(skill[5]),
	particleTimerTarget(skill[6]),
	particleTimerPreDelay(skill[7]),
	particleTimerVariable1(skill[8]),
	particleTimerVariable2(skill[9]),
	pedestalHasOrb(skill[0]),
	pedestalOrbType(skill[1]),
	pedestalInvertedPower(skill[3]),
	pedestalInGround(skill[4]),
	pedestalInit(skill[5]),
	pedestalAmbience(skill[6]),
	pedestalLockOrb(skill[7]),
	pedestalPowerStatus(skill[8]),
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
	portalCustomLevelsToJump(skill[6]),
	portalCustomRequiresPower(skill[7]),
	portalCustomSprite(skill[8]),
	portalCustomSpriteAnimationFrames(skill[9]),
	portalCustomZOffset(skill[10]),
	portalCustomLevelText1(skill[11]),
	portalCustomLevelText2(skill[12]),
	portalCustomLevelText3(skill[13]),
	portalCustomLevelText4(skill[14]),
	portalCustomLevelText5(skill[15]),
	portalCustomLevelText6(skill[16]),
	portalCustomLevelText7(skill[17]),
	portalCustomLevelText8(skill[18]),
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
	shrineSpellEffect(skill[0]),
	shrineRefire1(skill[1]),
	shrineRefire2(skill[3]),
	shrineDir(skill[4]),
	shrineAmbience(skill[5]),
	shrineInit(skill[6]),
	shrineActivateDelay(skill[7]),
	shrineZ(skill[8]),
	shrineDestXOffset(skill[9]),
	shrineDestYOffset(skill[10]),
	shrineDaedalusState(skill[11]),
	ceilingTileModel(skill[0]),
	ceilingTileDir(skill[1]),
	ceilingTileAllowTrap(skill[3]),
	ceilingTileBreakable(skill[4]),
	floorDecorationModel(skill[0]),
	floorDecorationRotation(skill[1]),
	floorDecorationHeightOffset(skill[3]),
	floorDecorationXOffset(skill[4]),
	floorDecorationYOffset(skill[5]),
	floorDecorationInteractText1(skill[8]),
	floorDecorationInteractText2(skill[9]),
	floorDecorationInteractText3(skill[10]),
	floorDecorationInteractText4(skill[11]),
	floorDecorationInteractText5(skill[12]),
	floorDecorationInteractText6(skill[13]),
	floorDecorationInteractText7(skill[14]),
	floorDecorationInteractText8(skill[15]),
	colliderDecorationModel(skill[0]),
	colliderDecorationRotation(skill[1]),
	colliderDecorationHeightOffset(skill[3]),
	colliderDecorationXOffset(skill[4]),
	colliderDecorationYOffset(skill[5]),
	colliderHasCollision(skill[6]),
	colliderSizeX(skill[7]),
	colliderSizeY(skill[8]),
	colliderMaxHP(skill[9]),
	colliderDiggable(skill[10]),
	colliderDamageTypes(skill[11]),
	colliderCurrentHP(skill[12]),
	colliderOldHP(skill[13]),
	colliderInit(skill[14]),
	colliderContainedEntity(skill[15]),
	colliderHideMonster(skill[16]),
	colliderKillerUid(skill[17]),
	furnitureType(skill[0]),
	furnitureInit(skill[1]),
	furnitureDir(skill[3]),
	furnitureHealth(skill[4]),
	furnitureMaxHealth(skill[9]),
	furnitureTableRandomItemChance(skill[10]),
	furnitureTableSpawnChairs(skill[11]),
	furnitureOldHealth(skill[15]),
	pistonCamDir(skill[0]),
	pistonCamTimer(skill[1]),
	pistonCamRotateSpeed(fskill[0]),
	arrowPower(skill[3]),
	arrowPoisonTime(skill[4]),
	arrowArmorPierce(skill[5]),
	arrowSpeed(fskill[4]),
	arrowFallSpeed(fskill[5]),
	arrowBoltDropOffRange(skill[6]),
	arrowShotByWeapon(skill[7]),
	arrowQuiverType(skill[8]),
	arrowShotByParent(skill[9]),
	arrowDropOffEquipmentModifier(skill[14]),
	actmagicIsVertical(skill[6]),
	actmagicIsOrbiting(skill[7]),
	actmagicOrbitDist(skill[8]),
	actmagicOrbitVerticalDirection(skill[9]),
	actmagicOrbitLifetime(skill[10]),
	actmagicMirrorReflected(skill[24]),
	actmagicMirrorReflectedCaster(skill[12]),
	actmagicCastByMagicstaff(skill[13]),
	actmagicOrbitVerticalSpeed(fskill[2]),
	actmagicOrbitStartZ(fskill[3]),
	actmagicOrbitStationaryX(fskill[4]),
	actmagicOrbitStationaryY(fskill[5]),
	actmagicOrbitStationaryCurrentDist(fskill[6]),
	actmagicSprayGravity(fskill[7]),
	actmagicOrbitStationaryHitTarget(skill[14]),
	actmagicOrbitHitTargetUID1(skill[15]),
	actmagicOrbitHitTargetUID2(skill[16]),
	actmagicOrbitHitTargetUID3(skill[17]),
	actmagicOrbitHitTargetUID4(skill[18]),
	actmagicProjectileArc(skill[19]),
	actmagicOrbitCastFromSpell(skill[20]),
	actmagicSpellbookBonus(skill[21]),
	actmagicCastByTinkerTrap(skill[22]),
	actmagicTinkerTrapFriendlyFire(skill[23]),
	actmagicReflectionCount(skill[25]),
	actmagicFromSpellbook(skill[26]),
	actmagicSpray(skill[27]),
	actmagicEmitter(skill[29]),
	goldAmount(skill[0]),
	goldAmbience(skill[1]),
	goldSokoban(skill[2]),
	goldBouncing(skill[3]),
	goldInContainer(skill[4]),
	interactedByMonster(skill[47]),
	highlightForUI(fskill[29]),
	highlightForUIGlow(fskill[28]),
	grayscaleGLRender(fskill[27]),
	noColorChangeAllyLimb(fskill[26]),
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
	textSourceIsScript(skill[3]),
	textSourceBegin(skill[4]),
	signalActivateDelay(skill[1]),
	signalTimerInterval(skill[2]),
	signalTimerRepeatCount(skill[3]),
	signalTimerLatchInput(skill[4]),
	signalInputDirection(skill[5]),
	signalGateANDPowerCount(skill[9]),
	signalInvertOutput(skill[10]),
	effectPolymorph(skill[50]),
	effectShapeshift(skill[53]),
	entityShowOnMap(skill[59]),
	thrownProjectilePower(skill[19]),
	thrownProjectileCharge(skill[20]),
	playerStartDir(skill[1]),
	pressurePlateTriggerType(skill[3]),
	worldTooltipAlpha(fskill[0]),
	worldTooltipZ(fskill[1]),
	worldTooltipActive(skill[0]),
	worldTooltipPlayer(skill[1]),
	worldTooltipInit(skill[3]),
	worldTooltipFadeDelay(skill[4]),
	worldTooltipIgnoreDrawing(skill[5]),
	worldTooltipRequiresButtonHeld(skill[6]),
	statueInit(skill[0]),
	statueDir(skill[1]),
	statueId(skill[3])
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

	if ( mynode )
	{
		mynode->element = this;
		mynode->deconstructor = &entityDeconstructor;
		mynode->size = sizeof(Entity);
	}

	myCreatureListNode = nullptr;
	if ( creaturelist )
	{
		addToCreatureList(creaturelist);
	}
	myWorldUIListNode = nullptr;
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
	lerpCurrentState.resetMovement();
	lerpCurrentState.resetPosition();
	lerpPreviousState.resetMovement();
	lerpPreviousState.resetPosition();
	lerpRenderState.resetMovement();
	lerpRenderState.resetPosition();
	bNeedsRenderPositionInit = true;
	bUseRenderInterpolation = false;
	mapGenerationRoomX = 0;
	mapGenerationRoomY = 0;
	lerp_ox = 0.0;
	lerp_oy = 0.0;
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
	for ( c = 0; c < 24; c++ )
	{
		flags[c] = false;
	}
	if ( entlist != nullptr && entlist == map.entities )
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
	/*if ( checkSpriteType(this->sprite) > 1 )
	{
		setSpriteAttributes(this, nullptr, nullptr);
	}*/

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
	if ( myWorldUIListNode )
	{
		list_RemoveNode(myWorldUIListNode);
		myWorldUIListNode = nullptr;
	}
	if ( myTileListNode )
	{
		list_RemoveNode(myTileListNode);
		myTileListNode = nullptr;
	}

#ifdef USE_FMOD
	if ( entity_sound )
	{
		entity_sound->stop();
		entity_sound = nullptr;
	}
#endif

	// alert clients of the entity's deletion
	if ( multiplayer == SERVER && !loading )
	{
		if ( mynode && mynode->list == map.entities && uid != 0 && flags[NOUPDATE] == false )
		{
			for ( i = 1; i < MAXPLAYERS; ++i )
			{
				if ( client_disconnected[i] == true || players[i]->isLocalPlayer() )
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
		if ( players[i] )
		{
			if ( this == players[i]->entity )
			{
				players[i]->entity = nullptr;    //TODO: PLAYERSWAP VERIFY. Should this do anything to the player itself?
				players[i]->cleanUpOnEntityRemoval();
			}
			if ( this == players[i]->ghost.my )
			{
				players[i]->ghost.my = nullptr;
				players[i]->ghost.reset();
			}
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
	if ( entity_rng )
	{
		delete entity_rng;
	}
}

/*-------------------------------------------------------------------------------

Entity::setObituary

Sets the obituary text on an entity.

-------------------------------------------------------------------------------*/

void Entity::setObituary(const char* obituary)
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
	if ( !hitstats )
	{
	    return;
	}

	hitstats->killer_uid = this->getUID();

	if ( behavior == &actMagicTrap )
	{
	    hitstats->killer = KilledBy::TRAP_MAGIC;
	    victim->setObituary(Language::get(1501));
	    return;
	}
	if ( behavior == &::actMagicTrapCeiling )
	{
	    hitstats->killer = KilledBy::TRAP_MAGIC;
	    victim->setObituary(Language::get(1502));
	    return;
	}
	if ( behavior == &actBoulder )
	{
	    hitstats->killer = KilledBy::BOULDER;
		if ( sprite == 989 )
		{
			victim->setObituary(Language::get(3898));
		}
		else if ( sprite == 990 )
		{
			victim->setObituary(Language::get(3899));
		}
		return;
	}

	Stat* myStats = this->getStats();
	if ( !myStats )
	{
		return;
	}

	if ( myStats->type == hitstats->type )
	{
		hitstats->killer = KilledBy::ALLY_BETRAYAL;
		if ( hitstats->sex == MALE )
		{
			snprintf(tempstr, 256, Language::get(1509), getMonsterLocalizedName(hitstats->type).c_str());
		}
		else
		{
			snprintf(tempstr, 256, Language::get(1510), getMonsterLocalizedName(hitstats->type).c_str());
		}
		victim->setObituary(tempstr);
	}
	else
	{
		hitstats->killer = KilledBy::MONSTER;
		switch ( myStats->type )
		{
			case HUMAN:
				victim->setObituary(Language::get(1511));
				break;
			case RAT:
				victim->setObituary(Language::get(1512));
				break;
			case GOBLIN:
				victim->setObituary(Language::get(1513));
				break;
			case SLIME:
				victim->setObituary(Language::get(1514));
				break;
			case TROLL:
				victim->setObituary(Language::get(1515));
				break;
			case SPIDER:
			    if (arachnophobia_filter) {
				    victim->setObituary(Language::get(4090));
			    } else {
				    victim->setObituary(Language::get(1516));
			    }
				break;
			case GHOUL:
				victim->setObituary(Language::get(1517));
				break;
			case SKELETON:
				victim->setObituary(Language::get(1518));
				break;
			case SCORPION:
				victim->setObituary(Language::get(1519));
				break;
			case CREATURE_IMP:
				victim->setObituary(Language::get(1520));
				break;
			case GNOME:
				victim->setObituary(Language::get(1521));
				break;
			case DEMON:
				victim->setObituary(Language::get(1522));
				break;
			case SUCCUBUS:
				victim->setObituary(Language::get(1523));
				break;
			case LICH:
				victim->setObituary(Language::get(1524));
				break;
			case MINOTAUR:
				victim->setObituary(Language::get(1525));
				break;
			case DEVIL:
				victim->setObituary(Language::get(1526));
				break;
			case SHOPKEEPER:
				if ( victim->behavior == &actPlayer )
				{
					if ( hitstats->type != HUMAN )
					{
						snprintf(hitstats->obituary, 127, Language::get(3244), getMonsterLocalizedPlural(hitstats->type).c_str(), myStats->name);
						hitstats->killer = KilledBy::TRESPASSING;
					}
					else
					{
						victim->setObituary(Language::get(1527)); // attempts a robbery.
						hitstats->killer = KilledBy::ATTEMPTED_ROBBERY;
					}
				}
				else
				{
					victim->setObituary(Language::get(1527)); // attempts a robbery.
					hitstats->killer = KilledBy::ATTEMPTED_ROBBERY;
				}
				break;
			case KOBOLD:
				victim->setObituary(Language::get(2150));
				break;
			case SCARAB:
				victim->setObituary(Language::get(2151));
				break;
			case CRYSTALGOLEM:
				victim->setObituary(Language::get(2152));
				break;
			case INCUBUS:
				victim->setObituary(Language::get(2153));
				break;
			case VAMPIRE:
				victim->setObituary(Language::get(2154));
				break;
			case SHADOW:
				victim->setObituary(Language::get(2155));
				break;
			case COCKATRICE:
				victim->setObituary(Language::get(2156));
				break;
			case INSECTOID:
				victim->setObituary(Language::get(2157));
				break;
			case GOATMAN:
				victim->setObituary(Language::get(2158));
				break;
			case AUTOMATON:
				victim->setObituary(Language::get(2159));
				break;
			case LICH_ICE:
				victim->setObituary(Language::get(2160));
				break;
			case LICH_FIRE:
				victim->setObituary(Language::get(2161));
				break;
			case SENTRYBOT:
				victim->setObituary(Language::get(2162));
				break;
			case SPELLBOT:
				victim->setObituary(Language::get(2163));
				break;
			case GYROBOT:
				victim->setObituary(Language::get(2164));
				break;
			case DUMMYBOT:
				victim->setObituary(Language::get(2165));
				break;
			case MIMIC:
				victim->setObituary(Language::get(2166));
				break;
			case BAT_SMALL:
				victim->setObituary(Language::get(6254));
				break;
			case BUGBEAR:
				victim->setObituary(Language::get(6255));
				break;
			default:
				victim->setObituary(Language::get(1500));
				break;
		}
	}
	hitstats->killer_monster = myStats->type;
	hitstats->killer_name = myStats->name;
}

/*-------------------------------------------------------------------------------

Entity::light

Returns the illumination of the given entity

-------------------------------------------------------------------------------*/

int Entity::entityLight()
{
	if ( this->x < 0 || this->y < 0 || this->x >= map.width << 4 || this->y >= map.height << 4 )
	{
		return 255;
	}
	int light_x = (int)this->x / 16;
	int light_y = (int)this->y / 16;
    const auto& light = lightmaps[0][light_y + light_x * map.height];
    //return (light.x + light.y + light.z) / 3.f;
    return std::min(std::max(0, (int)((light.x + light.y + light.z) / 3.f)), 255);
	//return std::min(std::max(0, (int)((light.x + light.y + light.z) / 3.f * 255.f)), 255);
}

/*-------------------------------------------------------------------------------

Entity::entityLightAfterReductions

Returns new entities' illumination,  
after reductions depending on the entity stats and another entity observing

-------------------------------------------------------------------------------*/

int Entity::entityLightAfterReductions(Stat& myStats, Entity* observer)
{
	int player = -1;
	const int minLight = (int)(TOUCHRANGE * 1.5);
	int light = std::max(minLight, entityLight()); // max 255 light to start with.
	bool invis = isInvisible();
	if ( !invis )
	{
		bool sneaking = false;
		if ( behavior == &actPlayer )
		{
			player = skill[2];
			if ( player > -1 && stats[player] )
			{
				if ( stats[player]->sneaking == 1 && !stats[player]->defending )
				{
					sneaking = true;
				}
			}
		}

		if ( observer )
		{
			light += observer->getPER() * 4; // add light level for PER x 4
			if ( sneaking )
			{
				light /= 2; // halve for sneaking
			}
			light -= (std::max(0, light - TOUCHRANGE)) * (1.0 * (myStats.getModifiedProficiency(PRO_STEALTH) / 100.0)); // reduce to 32 as sneak approaches 100
			Stat* observerStats = observer->getStats();
			if ( observerStats && observerStats->EFFECTS[EFF_BLIND] )
			{
				light = TOUCHRANGE;
			}
			if ( observer->behavior == &actMonster
				&& observer->monsterLastDistractedByNoisemaker > 0 
				&& uidToEntity(observer->monsterLastDistractedByNoisemaker) )
			{
				if ( observer->monsterTarget == observer->monsterLastDistractedByNoisemaker
					|| myStats.EFFECTS[EFF_DISORIENTED] )
				{
					// currently hunting noisemaker.
					light = 16;
				}
			}
		}
		else
		{
			if ( sneaking )
			{
				light /= 2; // halve for sneaking
			}
			light -= (std::max(0, light - TOUCHRANGE)) * (1.0 * (myStats.getModifiedProficiency(PRO_STEALTH) / 100.0)); // reduce to 32 as sneak approaches 100
		}
	}
	
	if ( invis )
	{
		light = std::min(light, TOUCHRANGE);
	}

	light = std::max(light, 0);
	if ( myStats.type == DUMMYBOT )
	{
		light = std::max(light, 256); // dummybots can always be seen at least 16 tiles away.
	}
	return light;
}

/*-------------------------------------------------------------------------------

Entity::effectTimes

Counts down effect timers and toggles effects whose timers reach zero - server only function

-------------------------------------------------------------------------------*/

void Entity::effectTimes()
{
	Stat* myStats = this->getStats();
	int player;
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
	spell_t* amplifyMagic_hijacked = nullptr;
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
			if ( player > 0 && multiplayer == SERVER && !players[player]->isLocalPlayer() )
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

		bool unsustain = false;
		switch ( spell->ID )
		{
			case SPELL_INVISIBILITY:
				invisibility_hijacked = spell;
				if ( !myStats->EFFECTS[EFF_INVISIBLE] )
				{
					for ( int c = 0; c < MAXPLAYERS; ++c )
					{
						if ( players[c] && players[c]->entity && players[c]->entity == uidToEntity(spell->caster) )
						{
							messagePlayer(c, MESSAGE_COMBAT, Language::get(591));    //If cure ailments or somesuch bombs the status effects.
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
					unsustain = true;
					list_RemoveNode(node); //Remove this here node.
					node = temp;
				}
				break;
			case SPELL_LEVITATION:
				levitation_hijacked = spell;
				if ( !myStats->EFFECTS[EFF_LEVITATING] )
				{
					for ( int c = 0; c < MAXPLAYERS; ++c )
					{
						if ( players[c] && players[c]->entity && players[c]->entity == uidToEntity(spell->caster) )
						{
							messagePlayer(c, MESSAGE_COMBAT, Language::get(592));
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
					unsustain = true;
					list_RemoveNode(node); //Remove this here node.
					node = temp;
				}
				break;
			case SPELL_REFLECT_MAGIC:
				reflectMagic_hijacked = spell;
				if ( !myStats->EFFECTS[EFF_MAGICREFLECT] )
				{
					for ( int c = 0; c < MAXPLAYERS; ++c )
					{
						if ( players[c] && players[c]->entity && players[c]->entity == uidToEntity(spell->caster) )
						{
							messagePlayer(c, MESSAGE_COMBAT, Language::get(2446));
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
					unsustain = true;
					list_RemoveNode(node); //Remove this here node.
					node = temp;
				}
				break;
			case SPELL_AMPLIFY_MAGIC:
				amplifyMagic_hijacked = spell;
				if ( !myStats->EFFECTS[EFF_MAGICAMPLIFY] )
				{
					for ( int c = 0; c < MAXPLAYERS; ++c )
					{
						if ( players[c] && players[c]->entity && players[c]->entity == uidToEntity(spell->caster) )
						{
							messagePlayer(c, MESSAGE_COMBAT, Language::get(3441));
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
					unsustain = true;
					list_RemoveNode(node); //Remove this here node.
					node = temp;
				}
				break;
			case SPELL_VAMPIRIC_AURA:
				vampiricAura_hijacked = spell;
				if ( !myStats->EFFECTS[EFF_VAMPIRICAURA] )
				{
					for ( int c = 0; c < MAXPLAYERS; ++c )
					{
						if ( players[c] && players[c]->entity && players[c]->entity == uidToEntity(spell->caster) )
						{
							messagePlayer(c, MESSAGE_COMBAT, Language::get(2447));
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
					unsustain = true;
					list_RemoveNode(node); //Remove this here node.
					node = temp;
				}
				break;
			default:
				//Unknown spell, undefined effect. Like, say, a fireball spell wound up in here for some reason. That's a nono.
				printlog("[entityEffectTimes] Warning: magic_effects spell that's not relevant. Should not be in the magic_effects list!\n");
				list_RemoveNode(node);
		}

		if ( unsustain )
		{
			// the node has been removed, tell the client to unsustain in their list.
			if ( player > 0 && multiplayer == SERVER && !players[player]->isLocalPlayer() )
			{
				strcpy((char*)net_packet->data, "UNCH");
				net_packet->data[4] = player;
				SDLNet_Write32(spell->ID, &net_packet->data[5]);
				net_packet->address.host = net_clients[player - 1].host;
				net_packet->address.port = net_clients[player - 1].port;
				net_packet->len = 9;
				sendPacketSafe(net_sock, -1, net_packet, player - 1);
			}
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
	spell_t* unsustainSpell = nullptr;

	for ( int c = 0; c < NUMEFFECTS; c++ )
	{
		if ( myStats->EFFECTS_TIMERS[c] > 0 )
		{
			myStats->EFFECTS_TIMERS[c]--;
			if ( c == EFF_POLYMORPH )
			{
				if ( myStats->EFFECTS_TIMERS[c] == TICKS_PER_SECOND * 15 )
				{
					playSoundPlayer(player, 611, 192);
					messagePlayer(player, MESSAGE_STATUS, Language::get(3193));
				}
			}
			else if ( c == EFF_SHAPESHIFT )
			{
				if ( myStats->EFFECTS_TIMERS[c] == TICKS_PER_SECOND * 15 )
				{
					playSoundPlayer(player, 611, 192);
					messagePlayer(player, MESSAGE_STATUS, Language::get(3475));
				}
			}
			if ( myStats->EFFECTS_TIMERS[c] == 0 )
			{
				myStats->EFFECTS[c] = false;
				switch ( c )
				{
					case EFF_ASLEEP:
						messagePlayer(player, MESSAGE_STATUS, Language::get(593));
						if ( monsterAllyGetPlayerLeader() && monsterAllySpecial == ALLY_SPECIAL_CMD_REST )
						{
							monsterAllySpecial = ALLY_SPECIAL_CMD_NONE;
							myStats->EFFECTS[EFF_HP_REGEN] = false;
							myStats->EFFECTS_TIMERS[EFF_HP_REGEN] = 0;
						}
						break;
					case EFF_MIMIC_LOCKED:
						if ( myStats->type == MIMIC )
						{
							monsterHitTime = std::max(HITRATE - 12, monsterHitTime); // ready to attack
							myStats->monsterMimicLockedBy = 0;
						}
						break;
					case EFF_HP_REGEN:
						//messagePlayer(player, MESSAGE_STATUS, Language::get(3476));
						updateClient = true;
						break;
					case EFF_MP_REGEN:
						//messagePlayer(player, MESSAGE_STATUS, Language::get(3477));
						updateClient = true;
						break;
					case EFF_POISONED:
						if ( myStats->mask != nullptr && myStats->mask->type == MASK_PLAGUE
							&& !(myStats->type != HUMAN && effectShapeshift != NOTHING)
							&& !(myStats->mask->beatitude >= 0 || shouldInvertEquipmentBeatitude(myStats)) )
						{
							// don't play any messages since we'll reset the counter in due time.
							// likely to happen on level change.
						}
						else
						{
							messagePlayer(player, MESSAGE_STATUS, Language::get(594));
						}
						break;
					case EFF_STUNNED:
						//messagePlayer(player, MESSAGE_STATUS, Language::get(595));
						break;
					case EFF_CONFUSED:
						messagePlayer(player, MESSAGE_STATUS, Language::get(596));
						break;
					case EFF_DRUNK:
						messagePlayer(player, MESSAGE_STATUS, Language::get(597));
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
								int oldMP = caster->getMP();
								bool deducted = caster->safeConsumeMP(1); //Consume 1 mana ever duration / mana seconds
								if ( deducted )
								{
									sustained = true;
									myStats->EFFECTS[c] = true;
									myStats->EFFECTS_TIMERS[c] = invisibility_hijacked->channel_duration;

									if ( caster->behavior == &actPlayer )
									{
										players[caster->skill[2]]->mechanics.sustainedSpellIncrementMP(oldMP - caster->getMP());
									}
								}
								else
								{
									int i = 0;
									for ( i = 0; i < MAXPLAYERS; ++i )
									{
										if ( players[i]->entity == caster )
										{
											messagePlayer(i, MESSAGE_STATUS, Language::get(598));
										}
									}
									unsustainSpell = invisibility_hijacked;
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
								messagePlayer(player, MESSAGE_STATUS, Language::get(599));
							}
						}
						break;
					case EFF_BLIND:
						if ( !this->isBlind() )
						{
							messagePlayer(player, MESSAGE_STATUS, Language::get(600));
						}
						else
						{
							messagePlayer(player, MESSAGE_STATUS, Language::get(601));
						}
						break;
					case EFF_GREASY:
						messagePlayer(player, MESSAGE_STATUS, Language::get(602));
						break;
					case EFF_MESSY:
						messagePlayer(player, MESSAGE_STATUS, Language::get(603));
						break;
					case EFF_FAST:
						messagePlayer(player, MESSAGE_STATUS, Language::get(604));
						break;
					case EFF_PARALYZED:
						messagePlayer(player, MESSAGE_STATUS, Language::get(605));
						break;
					case EFF_POTION_STR:
						messagePlayer(player, MESSAGE_STATUS, Language::get(3355));
						break;
					case EFF_AGILITY:
						messagePlayer(player, MESSAGE_STATUS, Language::get(6289));
						break;
					case EFF_CON_BONUS:
						messagePlayer(player, MESSAGE_STATUS, Language::get(6288));
						break;
					case EFF_PWR:
						messagePlayer(player, MESSAGE_STATUS, Language::get(6290));
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
								int oldMP = caster->getMP();
								bool deducted = caster->safeConsumeMP(1); //Consume 1 mana ever duration / mana seconds
								if ( deducted )
								{
									sustained = true;
									myStats->EFFECTS[c] = true;
									myStats->EFFECTS_TIMERS[c] = levitation_hijacked->channel_duration;

									if ( caster->behavior == &actPlayer )
									{
										players[caster->skill[2]]->mechanics.sustainedSpellIncrementMP(oldMP - caster->getMP());
									}
								}
								else
								{
									int i = 0;
									for ( i = 0; i < MAXPLAYERS; ++i )
									{
										if ( players[i]->entity == caster )
										{
											messagePlayer(i, MESSAGE_STATUS, Language::get(606));    //TODO: Unhardcode name?
										}
									}
									unsustainSpell = levitation_hijacked;
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
							if ( !isLevitating(myStats) )
							{
								messagePlayer(player, MESSAGE_STATUS, Language::get(607));
							}
						}
						break;
					case EFF_NAUSEA_PROTECTION:
						if ( myStats->mask != nullptr && myStats->mask->type == MASK_PLAGUE
							&& !(myStats->type != HUMAN && effectShapeshift != NOTHING) )
						{
							// don't play any messages since we'll reset the counter in due time.
							// likely to happen on level change.
						}
						else
						{
							setEffect(EFF_NAUSEA_PROTECTION, false, 0, true);
							if ( players[player]->entity->entityCanVomit() && !stats[player]->EFFECTS[EFF_VOMITING] )
							{
								messagePlayer(player, MESSAGE_STATUS, Language::get(634));
								players[player]->entity->char_gonnavomit = 140 + local_rng.rand() % 60;
							}
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
							messagePlayer(player, MESSAGE_STATUS, Language::get(608));
							if ( player >= 0 && players[player]->isLocalPlayer() )
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
						messagePlayer(player, MESSAGE_STATUS, Language::get(609));
						if ( myStats->HUNGER > getEntityHungerInterval(player, this, myStats, HUNGER_INTERVAL_OVERSATIATED) )
						{
							messagePlayer(player, MESSAGE_STATUS, Language::get(610));
						}
						else if ( myStats->HUNGER > getEntityHungerInterval(player, this, myStats, HUNGER_INTERVAL_WEAK)
							&& myStats->HUNGER <= getEntityHungerInterval(player, this, myStats, HUNGER_INTERVAL_HUNGRY) )
						{
							messagePlayer(player, MESSAGE_STATUS, Language::get(611));
							playSoundPlayer(player, 32, 128);
						}
						else if ( myStats->HUNGER > getEntityHungerInterval(player, this, myStats, HUNGER_INTERVAL_STARVING) 
							&& myStats->HUNGER <= getEntityHungerInterval(player, this, myStats, HUNGER_INTERVAL_WEAK) )
						{
							messagePlayer(player, MESSAGE_STATUS, Language::get(612));
							playSoundPlayer(player, 32, 128);
						}
						else if ( myStats->HUNGER <= getEntityHungerInterval(player, this, myStats, HUNGER_INTERVAL_STARVING) )
						{
							myStats->HUNGER = 50;
							messagePlayer(player, MESSAGE_STATUS, Language::get(613));
							playSoundPlayer(player, 32, 128);
						}
						serverUpdateHunger(player);
						break;
					case EFF_BLEEDING:
						messagePlayer(player, MESSAGE_STATUS, Language::get(614));
						break;
					case EFF_MAGICRESIST:
						messagePlayer(player, MESSAGE_STATUS, Language::get(2470));
						break;
					case EFF_FLUTTER:
						if ( !isLevitating(myStats) )
						{
							messagePlayer(player, MESSAGE_STATUS, Language::get(607));
							if ( behavior == &actPlayer 
								&& achievementObserver.playerAchievements[skill[2]].flutterShyCoordinates.first > 0.01
								&& achievementObserver.playerAchievements[skill[2]].flutterShyCoordinates.second > 0.01 )
							{
								int playerx = std::min(std::max<unsigned int>(1, this->x / 16), map.width - 2);
								int playery = std::min(std::max<unsigned int>(1, this->y / 16), map.height - 2);
								if ( map.tiles[0 + playery * MAPLAYERS + playerx * MAPLAYERS * map.height] )
								{
									// there's ground..
									achievementObserver.playerAchievements[skill[2]].checkPathBetweenObjects(this, nullptr, AchievementObserver::BARONY_ACH_FLUTTERSHY);
								}
							}
						}
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
								int oldMP = caster->getMP();
								bool deducted = caster->safeConsumeMP(1); //Consume 1 mana ever duration / mana seconds
								if ( deducted )
								{
									sustained = true;
									myStats->EFFECTS[c] = true;
									myStats->EFFECTS_TIMERS[c] = reflectMagic_hijacked->channel_duration;

									if ( caster->behavior == &actPlayer )
									{
										players[caster->skill[2]]->mechanics.sustainedSpellIncrementMP(oldMP - caster->getMP());
									}
								}
								else
								{
									int i = 0;
									for ( i = 0; i < MAXPLAYERS; ++i )
									{
										if ( players[i]->entity == caster )
										{
											messagePlayer(i, MESSAGE_STATUS, Language::get(2474));
										}
									}
									unsustainSpell = reflectMagic_hijacked;
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
							messagePlayer(player, MESSAGE_STATUS, Language::get(2471));
							updateClient = true;
						}
						break;
					case EFF_MAGICAMPLIFY:
						dissipate = true; //Remove the effect by default.
						if ( amplifyMagic_hijacked )
						{
							bool sustained = false;
							Entity* caster = uidToEntity(amplifyMagic_hijacked->caster);
							if ( caster )
							{
								//Deduct mana from caster. Cancel spell if not enough mana (simply leave sustained at false).
								int oldMP = caster->getMP();
								bool deducted = caster->safeConsumeMP(1); //Consume 1 mana ever duration / mana seconds
								if ( deducted )
								{
									sustained = true;
									myStats->EFFECTS[c] = true;
									myStats->EFFECTS_TIMERS[c] = amplifyMagic_hijacked->channel_duration;

									if ( caster->behavior == &actPlayer )
									{
										players[caster->skill[2]]->mechanics.sustainedSpellIncrementMP(oldMP - caster->getMP());
									}
								}
								else
								{
									int i = 0;
									for ( i = 0; i < MAXPLAYERS; ++i )
									{
										if ( players[i]->entity == caster )
										{
											messagePlayer(i, MESSAGE_STATUS, Language::get(3443));
										}
									}
									unsustainSpell = amplifyMagic_hijacked;
									list_RemoveNode(amplifyMagic_hijacked->magic_effects_node); //Remove it from the entity's magic effects. This has the side effect of removing it from the sustained spells list too.
								}
							}
							if ( sustained )
							{
								dissipate = false; //Sustained the spell, so do not stop being invisible.
							}
						}
						if ( dissipate )
						{
							messagePlayer(player, MESSAGE_STATUS, Language::get(3441));
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
								int oldMP = caster->getMP();
								bool deducted = caster->safeConsumeMP(1); //Consume 3 mana ever duration / mana seconds
								if ( deducted )
								{
									sustained = true;
									myStats->EFFECTS[c] = true;
									myStats->EFFECTS_TIMERS[c] = vampiricAura_hijacked->channel_duration;

									if ( caster->behavior == &actPlayer )
									{
										players[caster->skill[2]]->mechanics.sustainedSpellIncrementMP(oldMP - caster->getMP());
									}

									// monsters have a chance to un-sustain the spell each MP consume.
									if ( caster->behavior == &actMonster && local_rng.rand() % 20 == 0 )
									{
										sustained = false;
										list_RemoveNode(vampiricAura_hijacked->magic_effects_node);
									}
								}
								else
								{
									int i = 0;
									for ( i = 0; i < MAXPLAYERS; ++i )
									{
										if ( players[i]->entity == caster )
										{
											//messagePlayer(player, MESSAGE_STATUS, Language::get(2449));
										}
									}
									unsustainSpell = vampiricAura_hijacked;
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
							//if ( myStats->HUNGER > 250 )
							//{
							//	myStats->HUNGER = 252; // set to above 250 to trigger the hunger sound/messages when it decrements to 250.
							//	serverUpdateHunger(player);
							//}
							messagePlayer(player, MESSAGE_STATUS, Language::get(2449));
							updateClient = true;
						}
						break;
					case EFF_SLOW:
						messagePlayer(player, MESSAGE_STATUS, Language::get(604)); // "You return to your normal speed."
						break;
					case EFF_POLYMORPH:
						effectPolymorph = 0;
						serverUpdateEntitySkill(this, 50);
						if ( !myStats->EFFECTS[EFF_SHAPESHIFT] )
						{
							messagePlayer(player, MESSAGE_STATUS, Language::get(3185));
						}
						else
						{
							messagePlayer(player, MESSAGE_STATUS, Language::get(4303)); // wears out, no mention of 'normal' form
						}

						playSoundEntity(this, 400, 92);
						createParticleDropRising(this, 593, 1.f);
						serverSpawnMiscParticles(this, PARTICLE_EFFECT_RISING_DROP, 593);
						updateClient = true;
						break;
					case EFF_SHAPESHIFT:
						effectShapeshift = 0;
						serverUpdateEntitySkill(this, 53);
						if ( !myStats->EFFECTS[EFF_POLYMORPH] )
						{
							messagePlayer(player, MESSAGE_STATUS, Language::get(3417));
						}
						else
						{
							messagePlayer(player, MESSAGE_STATUS, Language::get(4302)); // return to your 'abnormal' form
						}

						playSoundEntity(this, 400, 92);
						createParticleDropRising(this, 593, 1.f);
						serverSpawnMiscParticles(this, PARTICLE_EFFECT_RISING_DROP, 593);
						updateClient = true;
						break;
					case EFF_TROLLS_BLOOD:
						messagePlayer(player, MESSAGE_STATUS, Language::get(3491));
						updateClient = true;
						break;
					case EFF_KNOCKBACK:
						break;
					case EFF_WITHDRAWAL:
						if ( player >= 0 && player < MAXPLAYERS )
						{
							if ( myStats->EFFECTS[EFF_DRUNK] )
							{
								// we still drunk! no need for hangover just yet...
								// extend another 15 seconds.
								myStats->EFFECTS_TIMERS[EFF_WITHDRAWAL] = TICKS_PER_SECOND * 15; 
							}
							else
							{
								playSoundPlayer(player, 32, 128);
								messagePlayer(player, MESSAGE_STATUS, Language::get(3247 + local_rng.rand() % 3));
								messagePlayer(player, MESSAGE_STATUS, Language::get(3222));
								this->setEffect(EFF_WITHDRAWAL, true, -2, true); // set effect as "active"
							}
						}
						break;
					case EFF_FEAR:
						this->monsterFearfulOfUid = 0;
						messagePlayer(player, MESSAGE_STATUS, Language::get(3439));
						updateClient = true;
						break;
					case EFF_PACIFY:
					case EFF_SHADOW_TAGGED:
					case EFF_WEBBED:
						updateClient = true;
						break;
					default:
						break;
				}
				if ( player > 0 && multiplayer == SERVER )
				{
					serverUpdateEffects(player);
				}
			}
			else if ( myStats->EFFECTS_TIMERS[c] == ((TICKS_PER_SECOND * 5) - 1) )
			{
				if ( player > 0 && multiplayer == SERVER )
				{
					serverUpdateEffects(player);
				}
			}
		}
		if ( unsustainSpell )
		{
			// we need to tell the client to un-sustain from their list.
			if ( player > 0 && multiplayer == SERVER && !players[player]->isLocalPlayer() )
			{
				strcpy((char*)net_packet->data, "UNCH");
				net_packet->data[4] = player;
				SDLNet_Write32(unsustainSpell->ID, &net_packet->data[5]);
				net_packet->address.host = net_clients[player - 1].host;
				net_packet->address.port = net_clients[player - 1].port;
				net_packet->len = 9;
				sendPacketSafe(net_sock, -1, net_packet, player - 1);
			}
		}
		unsustainSpell = nullptr;
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

bool Entity::increaseSkill(int skill, bool notify)
{
	Stat* myStats = this->getStats();
	int player = -1;

	if ( myStats == NULL )
	{
		return false;
	}
	if ( this->behavior == &actPlayer )
	{
		player = this->skill[2];

		if ( gameModeManager.currentSession.challengeRun.isActive()
			&& gameModeManager.currentSession.challengeRun.eventType == GameModeManager_t::CurrentSession_t::ChallengeRun_t::CHEVENT_NOSKILLS )
		{
			return false;
		}
	}

	bool increased = false;

	Uint32 color = makeColorRGB(255, 255, 0);
	if ( myStats->getProficiency(skill) < 100 )
	{
		myStats->setProficiency(skill, myStats->getProficiency(skill) + 1);
		if ( notify )
		{
			if ( player >= 0 )
			{
				messagePlayerColor(player, MESSAGE_SPAM_MISC, color, Language::get(615), getSkillLangEntry(skill));
				if ( players[player]->isLocalPlayer() )
				{
					skillUpAnimation[player].addSkillUp(skill, myStats->getProficiency(skill) - 1, 1);
				}
			}
		}
		switch ( myStats->getProficiency(skill) )
		{
			case 20:
				messagePlayerColor(player, MESSAGE_PROGRESSION, color, Language::get(616), getSkillLangEntry(skill));
				break;
			case 40:
				messagePlayerColor(player, MESSAGE_PROGRESSION, color, Language::get(617), getSkillLangEntry(skill));
				break;
			case 60:
				messagePlayerColor(player, MESSAGE_PROGRESSION, color, Language::get(618), getSkillLangEntry(skill));
				break;
			case 80:
				messagePlayerColor(player, MESSAGE_PROGRESSION, color, Language::get(619), getSkillLangEntry(skill));
				break;
			case 100:
				messagePlayerColor(player, MESSAGE_PROGRESSION, color, Language::get(620), getSkillLangEntry(skill));
				break;
			default:
				break;
		}

		if ( skill == PRO_SPELLCASTING && skillCapstoneUnlockedEntity(PRO_SPELLCASTING) )
		{
			//Spellcasting capstone = free casting of Forcebolt.
			//Give the player the spell if they haven't learned it yet.
			if ( player > 0 && multiplayer == SERVER && !players[player]->isLocalPlayer() )
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

		if ( skill == PRO_STEALTH && myStats->getProficiency(skill) == 100 )
		{
			if ( player >= 0 && client_classes[player] == CLASS_ACCURSED )
			{
				steamAchievementClient(player, "BARONY_ACH_BLOOD_RUNS_CLEAR");
			}
		}

		if ( player >= 0 && stats[player]->playerRace == RACE_GOBLIN && stats[player]->stat_appearance == 0
			&& myStats->getProficiency(skill) == 100 )
		{
			switch ( skill )
			{
				case PRO_SWORD:
				case PRO_POLEARM:
				case PRO_AXE:
				case PRO_MACE:
				case PRO_UNARMED:
					steamAchievementClient(player, "BARONY_ACH_SAVAGE");
					break;
				default:
					break;
			}
		}

		if ( skill == PRO_ALCHEMY )
		{
			if ( player >= 0 && players[player]->isLocalPlayer() )
			{
				GenericGUI[player].alchemyLearnRecipeOnLevelUp(myStats->getProficiency(skill));
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
			if ( player > 0 && multiplayer == SERVER && !players[player]->isLocalPlayer() )
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

		if ( player >= 0 )
		{
			Compendium_t::Events_t::eventUpdateCodex(player, Compendium_t::CPDM_XP_MAX_IN_FLOOR, "xp", 2, false, -1, true);
			Compendium_t::Events_t::eventUpdateCodex(player, Compendium_t::CPDM_XP_MAX_INSTANCE, "xp", 2);
			Compendium_t::Events_t::eventUpdateCodex(player, Compendium_t::CPDM_XP_SKILLS, "xp", 2);

			const char* skillstr = Compendium_t::getSkillStringForCompendium(skill);
			if ( strcmp(skillstr, "") )
			{
				if ( myStats->getProficiency(skill) == 100 )
				{
					Compendium_t::Events_t::eventUpdateCodex(player, Compendium_t::CPDM_CLASS_SKILL_LEGENDS, skillstr, 1);
				}
				if ( myStats->getProficiency(skill) == 20 )
				{
					Compendium_t::Events_t::eventUpdateCodex(player, Compendium_t::CPDM_CLASS_SKILL_NOVICES, skillstr, 1);
				}
				Compendium_t::Events_t::eventUpdateCodex(player, Compendium_t::CPDM_CLASS_SKILL_UPS, skillstr, 1);
				Compendium_t::Events_t::eventUpdateCodex(player, Compendium_t::CPDM_CLASS_SKILL_UPS_RUN_MAX, skillstr, 1);
				Compendium_t::Events_t::eventUpdateCodex(player, Compendium_t::CPDM_CLASS_SKILL_MAX, skillstr, myStats->getProficiency(skill));
			}
			if ( skill == PRO_STEALTH )
			{
				Compendium_t::Events_t::eventUpdateCodex(player, Compendium_t::CPDM_CLASS_SNEAK_SKILLUP_FLOOR, "sneaking", 1, false, -1, true);
			}
			Compendium_t::Events_t::eventUpdateCodex(player, Compendium_t::CPDM_CLASS_SKILL_UPS_ALL_RUN, "skills", 1);
		}

		increased = true;
	}

	int statBonusSkill = getStatForProficiency(skill);

	if ( statBonusSkill >= STAT_STR )
	{
		// stat has chance for bonus point if the relevant proficiency has been trained.
		// write the last proficiency that effected the skill.
		myStats->PLAYER_LVL_STAT_BONUS[statBonusSkill] = skill;
	}



	if ( player > 0 && multiplayer == SERVER && !players[player]->isLocalPlayer() )
	{
		// update SKILL
		strcpy((char*)net_packet->data, "SKIL");
		net_packet->data[4] = clientnum;
		net_packet->data[5] = skill;
		net_packet->data[6] = myStats->getProficiency(skill);
		if ( notify )
		{
			net_packet->data[6] |= (1 << 7);
		}
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
		net_packet->data[11] = (Uint8)myStats->EXP;
		net_packet->data[12] = (Uint8)myStats->LVL;
		SDLNet_Write16((Sint16)myStats->HP, &net_packet->data[13]);
		SDLNet_Write16((Sint16)myStats->MAXHP, &net_packet->data[15]);
		SDLNet_Write16((Sint16)myStats->MP, &net_packet->data[17]);
		SDLNet_Write16((Sint16)myStats->MAXMP, &net_packet->data[19]);
		net_packet->address.host = net_clients[player - 1].host;
		net_packet->address.port = net_clients[player - 1].port;
		net_packet->len = 21;
		sendPacketSafe(net_sock, -1, net_packet, player - 1);
	}

	return increased;
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
	strncpy(entitystats->obituary, Language::get(1500), 127);

	if ( this->behavior == &actPlayer && buddhamode && entitystats->HP < 1 )
	{
		entitystats->HP = 1; //Buddhas never die!
	}

	if ( this->behavior == &actPlayer && entitystats->OLDHP >= entitystats->HP )
	{
		inputs.addRumbleForPlayerHPLoss(skill[2], amount);
	}

	if ( multiplayer == SERVER )
	{
		for ( int i = 0; i < MAXPLAYERS; i++ )
		{
			if ( i > 0 && players[i] && this == players[i]->entity && !players[i]->isLocalPlayer() )
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
			if ( this->behavior == &actPlayer && abs(healthDiff) > 0 )
			{
				if ( serverSchedulePlayerHealthUpdate == 0 )
				{
					serverSchedulePlayerHealthUpdate = ::ticks;
				}
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

	if ( this->behavior == &actPlayer )
	{
		if ( godmode && amount < 0 )
		{
			amount = 0;
		}
		else if ( entitystats && entitystats->type == AUTOMATON && entitystats->HP <= 0 && this->playerAutomatonDeathCounter != 0 )
		{
			return;
		}
	}

	if ( !entitystats || amount == 0 )
	{
		if ( this->behavior == &actPlayer )
		{
			inputs.addRumbleForPlayerHPLoss(skill[2], amount);
		}
		return;
	}

	Sint32 oldHP = entitystats->HP;
	this->setHP(entitystats->HP + amount);
	if ( oldHP > entitystats->HP )
	{
		if ( behavior == &actPlayer )
		{
			Compendium_t::Events_t::eventUpdateCodex(skill[2], Compendium_t::CPDM_HP_LOST_RUN, "hp", oldHP - entitystats->HP);
			Compendium_t::Events_t::eventUpdateCodex(skill[2], Compendium_t::CPDM_HP_LOST_TOTAL, "hp", oldHP - entitystats->HP);
		}
	}
}

/*-------------------------------------------------------------------------------

Entity::setMP

sets the MP of the given entity

-------------------------------------------------------------------------------*/

void Entity::setMP(int amount, bool updateClients)
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

	if ( multiplayer == SERVER && updateClients )
	{
		for ( int i = 1; i < MAXPLAYERS; i++ )
		{
			if ( players[i] && this == players[i]->entity && !players[i]->isLocalPlayer() )
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

void Entity::modMP(int amount, bool updateClients)
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

	this->setMP(entitystats->MP + amount, updateClients);
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

void Entity::drainMP(int amount, bool notifyOverexpend)
{
	//A pointer to the entity's stats.
	Stat* entitystats = this->getStats();

	//Check if no stats found.
	if ( entitystats == NULL || amount == 0 )
	{
		return;
	}

	int overdrawn = 0;
	Sint32 oldMP = entitystats->MP;
	entitystats->MP -= amount;
	int player = -1;
	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		if ( players[i] && this == players[i]->entity )
		{
			player = i; //Set the player.
		}
	}

	if ( player >= 0 && entitystats->playerRace == RACE_INSECTOID && entitystats->stat_appearance == 0 )
	{
		if ( svFlags & SV_FLAG_HUNGER )
		{
			// we cast a spell or forcibly reduced our MP. therefore our hunger should reduce to match the MP value.
			if ( amount > 0 )
			{
				Sint32 hungerPointPerMana = playerInsectoidHungerValueOfManaPoint(*entitystats);
				Sint32 oldHunger = entitystats->HUNGER;
				entitystats->HUNGER -= amount * hungerPointPerMana;
				entitystats->HUNGER = std::max(0, entitystats->HUNGER);
				if ( player > 0 )
				{
					serverUpdateHunger(player);
				}
			}
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
		for ( int i = 1; i < MAXPLAYERS; ++i )
		{
			if ( players[i] && this == players[i]->entity && !players[i]->isLocalPlayer() )
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

	if ( player >= 0 )
	{
		Compendium_t::Events_t::eventUpdateCodex(player, Compendium_t::CPDM_MP_SPENT_RUN, "mp", oldMP - entitystats->MP);
		Compendium_t::Events_t::eventUpdateCodex(player, Compendium_t::CPDM_MP_SPENT_TOTAL, "mp", oldMP - entitystats->MP);
	}

	if ( overdrawn < 0 )
	{
		if ( player >= 0 && notifyOverexpend )
		{
			Uint32 color = makeColorRGB(255, 255, 0);
			messagePlayerColor(player, MESSAGE_STATUS, color, Language::get(621));
		}
		this->modHP(overdrawn); //Drain the extra magic from health.
		Stat* tempStats = this->getStats();
		if ( tempStats )
		{
			if ( tempStats->sex == MALE )
			{
				this->setObituary(Language::get(1528));
			}
			else
			{
				this->setObituary(Language::get(1529));
			}
			tempStats->killer = KilledBy::FAILED_INVOCATION;
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
		if ( behavior == &actPlayer && stat->type == VAMPIRE )
		{
			int HP = stat->HP;
			this->drainMP(amount, false);
			if ( (HP - stat->HP > 0) && (stat->HP % 5 == 0) )
			{
				Uint32 color = makeColorRGB(255, 255, 0);
				messagePlayerColor(skill[2], MESSAGE_STATUS, color, Language::get(621));
			}
			return true;
		}
		return false;    //Not enough mana.
	}
	else
	{
		if ( behavior == &actPlayer && stat->playerRace == RACE_INSECTOID && stat->stat_appearance == 0 )
		{
			if ( svFlags & SV_FLAG_HUNGER )
			{
				// we cast a spell or forcibly reduced our MP. therefore our hunger should reduce to match the MP value.
				if ( amount > 0 )
				{
					Sint32 hungerPointPerMana = playerInsectoidHungerValueOfManaPoint(*stat);
					Sint32 oldHunger = stat->HUNGER;
					stat->HUNGER -= amount * hungerPointPerMana;
					stat->HUNGER = std::max(0, stat->HUNGER);
					if ( this->skill[2] > 0 )
					{
						serverUpdateHunger(this->skill[2]);
					}
				}
			}
		}
		if ( behavior == &actPlayer )
		{
			Compendium_t::Events_t::eventUpdateCodex(skill[2], Compendium_t::CPDM_MP_SPENT_RUN, "mp", amount);
			Compendium_t::Events_t::eventUpdateCodex(skill[2], Compendium_t::CPDM_MP_SPENT_TOTAL, "mp", amount);
		}
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

int Entity::getHungerTickRate(Stat* myStats, bool isPlayer, bool checkItemsEffects)
{
	int hungerTickRate = 30; // how many ticks to reduce hunger by a point.
	if ( !myStats )
	{
		return hungerTickRate;
	}
	int hungerring = 0;
	if ( checkItemsEffects )
	{
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
					if ( isPlayer && shouldInvertEquipmentBeatitude(myStats) )
					{
						hungerring = 1;
					}
					else
					{
						hungerring = -1;
					}
				}
			}
		}
	}

	int vampiricHunger = 0;
	if ( checkItemsEffects )
	{
		if ( myStats->EFFECTS[EFF_VAMPIRICAURA] )
		{
			if ( myStats->EFFECTS_TIMERS[EFF_VAMPIRICAURA] == -2 )
			{
				vampiricHunger = 2;
			}
			else
			{
				vampiricHunger = 1;
			}
		}
	}

	if ( !strncmp(map.name, "Sanctum", 7)
		|| !strncmp(map.name, "Boss", 4)
		|| !strncmp(map.name, "Hell Boss", 9)
		|| !strncmp(map.name, "Mages Guild", 11)
		|| strstr(map.name, " Transition")
		|| currentlevel == 0 )
	{
		hungerring = 1; // slow down hunger on boss stages.
		if ( vampiricHunger > 0 )
		{
			vampiricHunger *= 8;
		}
	}

	if ( vampiricHunger > 0 )
	{
		hungerTickRate = 5 * vampiricHunger;
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
	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		if ( !client_disconnected[i] )
		{
			++playerCount;
		}
	}

	if ( playerCount == 3 )
	{
		hungerTickRate *= 1.25;
	}
	else if ( playerCount == 4 )
	{
		hungerTickRate *= 1.5;
	}
	if ( myStats->type == INSECTOID )
	{
		hungerTickRate *= 1.5;
	}

	if ( checkItemsEffects )
	{
		if ( myStats->mask && myStats->mask->type == MASK_GRASS_SPRIG )
		{
			if ( !(isPlayer &&
				(myStats->type == TROLL || myStats->type == RAT || myStats->type == SPIDER || myStats->type == CREATURE_IMP)) )
			{
				if ( myStats->mask->beatitude >= 0 || shouldInvertEquipmentBeatitude(myStats) )
				{
					real_t mult = std::min(1.25 + (0.25 * abs(myStats->mask->beatitude)), 2.0);
					hungerTickRate *= mult;
				}
				else
				{
					real_t mult = std::max(0.25, 1.0 - (0.25 * abs(myStats->mask->beatitude)));
					hungerTickRate *= mult;
				}
			}
		}
	}

	bool playerAutomaton = (myStats->type == AUTOMATON && isPlayer);
	if ( playerAutomaton )
	{
		// give a little extra hunger duration.
		if ( playerCount == 3 )
		{
			hungerTickRate *= 1.25; // 1.55x (1.25 x 1.25)
		}
		else if ( playerCount == 4 )
		{
			hungerTickRate *= 1.5; // 2.55x (1.5 x 1.5)
		}

		if ( myStats->HUNGER > 1000 && hungerTickRate > 30 )
		{
			hungerTickRate = 30; // don't slow down during superheated.
		}
	}

	hungerTickRate = std::max(hungerTickRate, 1);
	return hungerTickRate;
}

void Entity::monsterRollLevelUpStats(int increasestat[3])
{
	// monsters use this.
	increasestat[0] = local_rng.rand() % 6;
	int r = local_rng.rand() % 6;
	while ( r == increasestat[0] ) {
		r = local_rng.rand() % 6;
	}
	increasestat[1] = r;
	r = local_rng.rand() % 6;
	while ( r == increasestat[0] || r == increasestat[1] ) {
		r = local_rng.rand() % 6;
	}
	increasestat[2] = r;
}

static ConsoleVariable<bool> cvar_noxp("/noxp", false);

void Entity::handleEffects(Stat* myStats)
{
	int increasestat[3] = { 0, 0, 0 };
	int i, c;
	int player = -1;

	if ( !myStats )
	{
		return;
	}
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

		if ( myStats->defending )
		{
			if ( myStats->shield && !myStats->EFFECTS[EFF_SHAPESHIFT] )
			{
				if ( players[player]->mechanics.defendTicks == 0 )
				{
					players[player]->mechanics.defendTicks = ::ticks;
				}
			}
			else
			{
				players[player]->mechanics.defendTicks = 0;
			}
		}
		else
		{
			players[player]->mechanics.defendTicks = 0;
		}
	}

	auto& camera_shakex = cameravars[player >= 0 ? player : 0].shakex;
	auto& camera_shakey = cameravars[player >= 0 ? player : 0].shakey;
	auto& camera_shakex2 = cameravars[player >= 0 ? player : 0].shakex2;
	auto& camera_shakey2 = cameravars[player >= 0 ? player : 0].shakey2;

	// sleep Zs
	if ( myStats->EFFECTS[EFF_ASLEEP] && ticks % 30 == 0 )
	{
		spawnSleepZ(this->x + cos(this->yaw) * 2, this->y + sin(this->yaw) * 2, this->z);
	}

	int startingHPInHandleEffects = myStats->HP;

	if ( creatureShadowTaggedThisUid != 0 )
	{
		Entity* tagged = uidToEntity(creatureShadowTaggedThisUid);
		if ( !tagged )
		{
			creatureShadowTaggedThisUid = 0;
			serverUpdateEntitySkill(this, 54);
		}
		else
		{
			Stat* tagStats = tagged->getStats();
			if ( tagStats && !tagStats->EFFECTS[EFF_SHADOW_TAGGED] ) // effect timed out.
			{
				creatureShadowTaggedThisUid = 0;
				serverUpdateEntitySkill(this, 54);
			}
		}
	}

	if ( *cvar_noxp )
	{
		myStats->EXP = 0;
	}

	// level ups
	if ( myStats->EXP >= 100 )
	{
		myStats->EXP -= 100;
		if ( player >= 0 )
		{
			if ( myStats->LVL < 255 )
			{
				myStats->LVL++;
			}
		}
		else
		{
			myStats->LVL++;
		}

		if ( player >= 0 && players[player]->isLocalPlayer() )
		{
			players[player]->hud.xpBar.animateState = Player::HUD_t::AnimateStates::ANIMATE_LEVELUP_RISING;
			players[player]->hud.xpBar.xpLevelups++;
		}

		Uint32 color = makeColorRGB(255, 255, 0);
		messagePlayerColor(player, MESSAGE_SPAM_MISC, color, Language::get(622));

		static ConsoleVariable<int> cvar_lvlup_ally_sfx("/lvlup_ally_sfx", 520);

		// increase MAXHP/MAXMP
		myStats->MAXHP += HP_MOD;
		modHP(getHPRestoreOnLevelUp());
		myStats->HP = std::min(myStats->HP, myStats->MAXHP);
		if ( !(behavior == &actMonster && monsterAllySummonRank != 0) )
		{
			myStats->MP += MP_MOD;
			myStats->MAXMP += MP_MOD;
			if ( behavior == &actPlayer && myStats->playerRace == RACE_INSECTOID && myStats->stat_appearance == 0 )
			{
				myStats->MAXMP = std::min(100, myStats->MAXMP);
				if ( svFlags & SV_FLAG_HUNGER )
				{
					Sint32 hungerPointPerMana = playerInsectoidHungerValueOfManaPoint(*myStats);
					myStats->HUNGER += MP_MOD * hungerPointPerMana;
					myStats->HUNGER = std::min(1000, myStats->HUNGER);
					serverUpdateHunger(skill[2]);
				}
			}
			myStats->MP = std::min(myStats->MP, myStats->MAXMP);
		}

		// now pick three attributes to increase

		if ( player >= 0 )
		{
			// players only.
			this->playerStatIncrease(client_classes[player], increasestat);
			Compendium_t::Events_t::eventUpdateCodex(player, Compendium_t::CPDM_CLASS_LVL_GAINED, "leveling up", 1);
			Compendium_t::Events_t::eventUpdateCodex(player, Compendium_t::CPDM_CLASS_LVL_MAX, "leveling up", myStats->LVL);
		}
		else if ( behavior == &actMonster && monsterAllySummonRank != 0 )
		{
			bool secondSummon = false;
			if ( MonsterData_t::nameMatchesSpecialNPCName(*myStats, "skeleton knight") )
			{
				this->playerStatIncrease(CLASS_WARRIOR, increasestat); // warrior weighting
			}
			else if ( MonsterData_t::nameMatchesSpecialNPCName(*myStats, "skeleton sentinel") )
			{
				secondSummon = true;
				this->playerStatIncrease(CLASS_ROGUE, increasestat); // rogue weighting
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
				else if ( myStats->LVL == 35 )
				{
					steamAchievementClient(this->monsterAllyIndex, "BARONY_ACH_BONE_TO_PICK");
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
							color = makeColorRGB(255, 255, 0);
							messagePlayerMonsterEvent(leader->skill[2], color, *myStats, Language::get(3197), Language::get(3197), MSG_GENERIC);
							playSoundPlayer(leader->skill[2], 40, 64);
						}
					}
				}
			}
		}
		else
		{
			// monsters use this.
			Entity::monsterRollLevelUpStats(increasestat);

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
							color = makeColorRGB(0, 255, 0);
							messagePlayerMonsterEvent(i, color, *myStats, Language::get(2379), Language::get(2379), MSG_GENERIC);
							playSoundEntity(this, *cvar_lvlup_ally_sfx, 128);
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

			std::vector<LevelUpAnimation_t::LevelUp_t::StatUp_t> StatUps;
			for ( i = 0; i < 3; i++ )
			{
				messagePlayerColor(player, MESSAGE_SPAM_MISC, color, Language::get(623 + increasestat[i]));
				switch ( increasestat[i] )
				{
					case STAT_STR: // STR
					{
						if ( myStats->STR < MAX_PLAYER_STAT_VALUE )
						{
							StatUps.push_back(LevelUpAnimation_t::LevelUp_t::StatUp_t(increasestat[i], myStats->STR, 1));
							int increment = 1;
							myStats->PLAYER_LVL_STAT_TIMER[increasestat[i]] = statIconTicks;
							if ( myStats->PLAYER_LVL_STAT_BONUS[increasestat[i]] >= PRO_LOCKPICKING && !rolledBonusStat )
							{
								if ( local_rng.rand() % 5 == 0 )
								{
									if ( (myStats->STR + 1) < MAX_PLAYER_STAT_VALUE )
									{
										StatUps.at(StatUps.size() - 1).increaseStat += 1;
										increment++;
										Compendium_t::Events_t::eventUpdateCodex(player, Compendium_t::CPDM_STAT_DOUBLED, "str", 1);
										rolledBonusStat = true;
										myStats->PLAYER_LVL_STAT_TIMER[increasestat[i] + NUMSTATS] = statIconTicks;
										//messagePlayer(0, "Rolled bonus in %d", increasestat[i]);
									}
								}
							}
							myStats->STR += increment;
							Compendium_t::Events_t::eventUpdateCodex(player, Compendium_t::CPDM_STAT_INCREASES, "str", 1);
							Compendium_t::Events_t::eventUpdateCodex(player, Compendium_t::CPDM_CLASS_STAT_STR_MAX, "str", myStats->STR);
						}
						break;
					}
					case STAT_DEX: // DEX
					{
						if ( myStats->DEX < MAX_PLAYER_STAT_VALUE )
						{
							StatUps.push_back(LevelUpAnimation_t::LevelUp_t::StatUp_t(increasestat[i], myStats->DEX, 1));
							int increment = 1;
							myStats->PLAYER_LVL_STAT_TIMER[increasestat[i]] = statIconTicks;
							if ( myStats->PLAYER_LVL_STAT_BONUS[increasestat[i]] >= PRO_LOCKPICKING && !rolledBonusStat )
							{
								if ( local_rng.rand() % 5 == 0 )
								{
									if ( (myStats->DEX + 1) < MAX_PLAYER_STAT_VALUE )
									{
										StatUps.at(StatUps.size() - 1).increaseStat += 1;
										increment++;
										Compendium_t::Events_t::eventUpdateCodex(player, Compendium_t::CPDM_STAT_DOUBLED, "dex", 1);
										rolledBonusStat = true;
										myStats->PLAYER_LVL_STAT_TIMER[increasestat[i] + NUMSTATS] = statIconTicks;
										//messagePlayer(0, "Rolled bonus in %d", increasestat[i]);
									}
								}
							}
							myStats->DEX += increment;
							Compendium_t::Events_t::eventUpdateCodex(player, Compendium_t::CPDM_STAT_INCREASES, "dex", increment);
							Compendium_t::Events_t::eventUpdateCodex(player, Compendium_t::CPDM_CLASS_STAT_DEX_MAX, "dex", myStats->DEX);
						}
						break;
					}
					case STAT_CON: // CON
					{
						if ( myStats->CON < MAX_PLAYER_STAT_VALUE )
						{
							StatUps.push_back(LevelUpAnimation_t::LevelUp_t::StatUp_t(increasestat[i], myStats->CON, 1));
							int increment = 1;
							myStats->PLAYER_LVL_STAT_TIMER[increasestat[i]] = statIconTicks;
							if ( myStats->PLAYER_LVL_STAT_BONUS[increasestat[i]] >= PRO_LOCKPICKING && !rolledBonusStat )
							{
								if ( local_rng.rand() % 5 == 0 )
								{
									if ( (myStats->CON + 1) < MAX_PLAYER_STAT_VALUE )
									{
										StatUps.at(StatUps.size() - 1).increaseStat += 1;
										increment++;
										Compendium_t::Events_t::eventUpdateCodex(player, Compendium_t::CPDM_STAT_DOUBLED, "con", 1);
										rolledBonusStat = true;
										myStats->PLAYER_LVL_STAT_TIMER[increasestat[i] + NUMSTATS] = statIconTicks;
										//messagePlayer(0, "Rolled bonus in %d", increasestat[i]);
									}
								}
							}
							myStats->CON += increment;
							Compendium_t::Events_t::eventUpdateCodex(player, Compendium_t::CPDM_STAT_INCREASES, "con", increment);
							Compendium_t::Events_t::eventUpdateCodex(player, Compendium_t::CPDM_CLASS_STAT_CON_MAX, "con", myStats->CON);
						}
						break;
					}
					case STAT_INT: // INT
					{
						if ( myStats->INT < MAX_PLAYER_STAT_VALUE )
						{
							StatUps.push_back(LevelUpAnimation_t::LevelUp_t::StatUp_t(increasestat[i], myStats->INT, 1));
							int increment = 1;
							myStats->PLAYER_LVL_STAT_TIMER[increasestat[i]] = statIconTicks;
							if ( myStats->PLAYER_LVL_STAT_BONUS[increasestat[i]] >= PRO_LOCKPICKING && !rolledBonusStat )
							{
								if ( local_rng.rand() % 5 == 0 )
								{
									if ( (myStats->INT + 1) < MAX_PLAYER_STAT_VALUE )
									{
										StatUps.at(StatUps.size() - 1).increaseStat += 1;
										increment++;
										Compendium_t::Events_t::eventUpdateCodex(player, Compendium_t::CPDM_STAT_DOUBLED, "int", 1);
										rolledBonusStat = true;
										myStats->PLAYER_LVL_STAT_TIMER[increasestat[i] + NUMSTATS] = statIconTicks;
										//messagePlayer(0, "Rolled bonus in %d", increasestat[i]);
									}
								}
							}
							myStats->INT += increment;
							Compendium_t::Events_t::eventUpdateCodex(player, Compendium_t::CPDM_STAT_INCREASES, "int", increment);
							Compendium_t::Events_t::eventUpdateCodex(player, Compendium_t::CPDM_CLASS_STAT_INT_MAX, "int", myStats->INT);
						}
						break;
					}
					case STAT_PER: // PER
					{
						if ( myStats->PER < MAX_PLAYER_STAT_VALUE )
						{
							StatUps.push_back(LevelUpAnimation_t::LevelUp_t::StatUp_t(increasestat[i], myStats->PER, 1));
							int increment = 1;
							myStats->PLAYER_LVL_STAT_TIMER[increasestat[i]] = statIconTicks;
							if ( myStats->PLAYER_LVL_STAT_BONUS[increasestat[i]] >= PRO_LOCKPICKING && !rolledBonusStat )
							{
								if ( local_rng.rand() % 5 == 0 )
								{
									if ( (myStats->PER + 1) < MAX_PLAYER_STAT_VALUE )
									{
										StatUps.at(StatUps.size() - 1).increaseStat += 1;
										increment++;
										rolledBonusStat = true;
										myStats->PLAYER_LVL_STAT_TIMER[increasestat[i] + NUMSTATS] = statIconTicks;
										//messagePlayer(0, "Rolled bonus in %d", increasestat[i]);
									}
								}
							}
							myStats->PER += increment;
							Compendium_t::Events_t::eventUpdateCodex(player, Compendium_t::CPDM_STAT_INCREASES, "per", increment);
							Compendium_t::Events_t::eventUpdateCodex(player, Compendium_t::CPDM_CLASS_STAT_PER_MAX, "per", myStats->PER);
						}
						break;
					}
					case STAT_CHR: // CHR
					{
						if ( myStats->CHR < MAX_PLAYER_STAT_VALUE )
						{
							StatUps.push_back(LevelUpAnimation_t::LevelUp_t::StatUp_t(increasestat[i], myStats->CHR, 1));
							int increment = 1;
							myStats->PLAYER_LVL_STAT_TIMER[increasestat[i]] = statIconTicks;
							if ( myStats->PLAYER_LVL_STAT_BONUS[increasestat[i]] >= PRO_LOCKPICKING && !rolledBonusStat )
							{
								if ( local_rng.rand() % 5 == 0 )
								{
									if ( (myStats->CHR + 1) < MAX_PLAYER_STAT_VALUE )
									{
										StatUps.at(StatUps.size() - 1).increaseStat += 1;
										increment++;
										Compendium_t::Events_t::eventUpdateCodex(player, Compendium_t::CPDM_STAT_DOUBLED, "chr", 1);
										rolledBonusStat = true;
										myStats->PLAYER_LVL_STAT_TIMER[increasestat[i] + NUMSTATS] = statIconTicks;
										//messagePlayer(0, "Rolled bonus in %d", increasestat[i]);
									}
								}
							}
							myStats->CHR += increment;
							Compendium_t::Events_t::eventUpdateCodex(player, Compendium_t::CPDM_STAT_INCREASES, "chr", increment);
							Compendium_t::Events_t::eventUpdateCodex(player, Compendium_t::CPDM_CLASS_STAT_CHR_MAX, "chr", myStats->CHR);
						}
						break;
					}
					default:
						break;
				}
			}

			for ( i = 0; i < MAXPLAYERS; ++i )
			{
				// broadcast a player levelled up to other players.
				if ( i != player )
				{
					if ( client_disconnected[i] || multiplayer == SINGLE )
					{
						continue;
					}
					messagePlayerMonsterEvent(i, color, *myStats, Language::get(2379), Language::get(2379), MSG_GENERIC, this);
				}
			}

			if ( players[player]->isLocalPlayer() )
			{
				levelUpAnimation[player].addLevelUp(stats[player]->LVL, 1, StatUps);
			}
		}

		// inform clients of stat changes
		if ( multiplayer == SERVER )
		{
			if ( player > 0 && !players[player]->isLocalPlayer() )
			{
				strcpy((char*)net_packet->data, "ATTR");
				net_packet->data[4] = clientnum;
				net_packet->data[5] = (Sint8)myStats->STR;
				net_packet->data[6] = (Sint8)myStats->DEX;
				net_packet->data[7] = (Sint8)myStats->CON;
				net_packet->data[8] = (Sint8)myStats->INT;
				net_packet->data[9] = (Sint8)myStats->PER;
				net_packet->data[10] = (Sint8)myStats->CHR;
				net_packet->data[11] = (Uint8)myStats->EXP;
				net_packet->data[12] = (Uint8)myStats->LVL;
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
	int hungerTickRate = Entity::getHungerTickRate(myStats, behavior == &actPlayer, true);
	int vampiricHunger = 0;
	if ( myStats->EFFECTS[EFF_VAMPIRICAURA] )
	{
		if ( myStats->EFFECTS_TIMERS[EFF_VAMPIRICAURA] == -2 )
		{
			vampiricHunger = 2;
		}
		else
		{
			vampiricHunger = 1;
		}
	}
	bool processHunger = (svFlags & SV_FLAG_HUNGER) && !MFLAG_DISABLEHUNGER; // check server flags if hunger is enabled.
	if ( player >= 0 )
	{
		if ( myStats->type == SKELETON || myStats->type == AUTOMATON )
		{
			processHunger = false;
		}
	}

	bool playerAutomaton = (myStats->type == AUTOMATON && player >= 0);

	if ( playerAutomaton )
	{
		if ( ticks % (hungerTickRate / 2) == 0 )
		{
			//messagePlayer(0, "hungertick %d, curr %d, players: %d", hungerTickRate, myStats->HUNGER, playerCount);
			if ( myStats->HUNGER > 0 )
			{
				bool update = (myStats->HUNGER % 100 == 0);
				if ( myStats->HUNGER > 300 && myStats->HUNGER <= 600 )
				{
					update = (myStats->HUNGER % 25 == 0); // critical levels for players to show hunger meter.
				}
				myStats->HUNGER--;
				if ( update )
				{
					serverUpdateHunger(player);
				}
				if ( myStats->HUNGER == 299 )
				{
					messagePlayer(player, MESSAGE_STATUS, Language::get(3708));
					messagePlayer(player, MESSAGE_STATUS, Language::get(3709));
					playSoundPlayer(player, 32, 128);
				}
				else if ( myStats->HUNGER == 0 )
				{
					messagePlayer(player, MESSAGE_STATUS, Language::get(3708));
					messagePlayer(player, MESSAGE_STATUS, Language::get(3710));
					playSoundPlayer(player, 32, 128);
				}
			}
			else
			{
				myStats->HUNGER = 0;
			}
		}
	}
	
	if ( !processHunger && !playerAutomaton )
	{
		if ( behavior == &actMonster )
		{
			myStats->HUNGER = 500;
		}
		else if ( myStats->HUNGER < 100 )
		{
			myStats->HUNGER = 100;
			serverUpdateHunger(player);
		}
		else if ( myStats->type == SKELETON && myStats->HUNGER > 1500 )
		{
			myStats->HUNGER = 1499;
			serverUpdateHunger(player);
		}
		if ( vampiricHunger > 0 )
		{
			if ( ticks % (TICKS_PER_SECOND * 25) == 0 )
			{
				this->modHP(-1);
				if ( myStats->HP <= 0 )
				{
					this->setObituary(Language::get(1530));
					myStats->killer = KilledBy::STARVATION;
				}

				// Give the Player feedback on being hurt
				playSoundEntity(this, 28, 64); // "Damage.ogg"

				if ( myStats->HP > 0 )
				{
					messagePlayer(player, MESSAGE_STATUS, Language::get(3253));

					// Shake the Host's screen
					if ( myStats->HP <= 10 )
					{
						if ( player >= 0 && players[player]->isLocalPlayer() )
						{
							camera_shakex += .1;
							camera_shakey += 10;
						}
						else if ( player > 0 && multiplayer == SERVER && !players[player]->isLocalPlayer() )
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
	}
	else
	{
		if ( myStats->HUNGER > 0 && !playerAutomaton )
		{
			if ( ticks % hungerTickRate == 0 )
			{
				//messagePlayer(0, "hungertick %d, curr %d, players: %d", hungerTickRate, myStats->HUNGER, playerCount);
				myStats->HUNGER--;
				Sint32 noLongerFull = getEntityHungerInterval(player, this, myStats, HUNGER_INTERVAL_OVERSATIATED);
				Sint32 youFeelHungry = getEntityHungerInterval(player, this, myStats, HUNGER_INTERVAL_HUNGRY);
				Sint32 youFeelWeak = getEntityHungerInterval(player, this, myStats, HUNGER_INTERVAL_WEAK);
				Sint32 youFeelFaint = getEntityHungerInterval(player, this, myStats, HUNGER_INTERVAL_STARVING);

				if ( myStats->HUNGER == noLongerFull )
				{
					if ( !myStats->EFFECTS[EFF_VOMITING] )
					{
						messagePlayer(player, MESSAGE_STATUS, Language::get(629));
					}
					serverUpdateHunger(player);
				}
				else if ( myStats->HUNGER == youFeelHungry )
				{
					if ( !myStats->EFFECTS[EFF_VOMITING] )
					{
						messagePlayer(player, MESSAGE_STATUS, Language::get(630));
						playSoundPlayer(player, 32, 128);
					}
					serverUpdateHunger(player);
				}
				else if ( myStats->HUNGER == youFeelWeak )
				{
					if ( !myStats->EFFECTS[EFF_VOMITING] )
					{
						messagePlayer(player, MESSAGE_STATUS, Language::get(631));
						playSoundPlayer(player, 32, 128);
					}
					serverUpdateHunger(player);
				}
				else if ( myStats->HUNGER == youFeelFaint )
				{
					if ( !myStats->EFFECTS[EFF_VOMITING] )
					{
						messagePlayer(player, MESSAGE_STATUS, Language::get(632));
						playSoundPlayer(player, 32, 128);
					}
					serverUpdateHunger(player);
				}
			}
		}
		else
		{
			bool doStarvation = true;
			// Process HUNGER Effect - Wasting Away
			if ( playerAutomaton )
			{
				if ( myStats->HUNGER == 0 && myStats->MP <= 0 )
				{
					// deal HP damage.
					/*if ( myStats->HUNGER > 1 )
					{
						myStats->HUNGER = 1;
					}*/
				}
				else
				{
					doStarvation = false;
				}
			}
			else
			{
				myStats->HUNGER = 0;
			}

			// Deal Hunger damage every three seconds
			if ( doStarvation && !myStats->EFFECTS[EFF_VOMITING] && ticks % 150 == 0 )
			{
				serverUpdateHunger(player);
				bool allowStarve = true;
				if ( playerAutomaton )
				{
					if ( !(svFlags & SV_FLAG_HUNGER) )
					{
						allowStarve = false; // hunger off, don't starve at 0 MP.
					}
				}

				if ( player >= 0 && allowStarve ) // Only Players can starve
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
						int damage = std::max(1, myStats->MAXHP / 20);
						this->modHP(-damage);

						if ( myStats->HP <= 0 )
						{
							if ( playerAutomaton )
							{
							    myStats->killer = KilledBy::NO_FUEL;
								this->setObituary(Language::get(3864));
								steamAchievementEntity(this, "BARONY_ACH_RUST_IN_PEACE");
							}
							else
							{
							    myStats->killer = KilledBy::STARVATION;
							    this->setObituary(Language::get(1530));
							}
						}
					}

					// Give the Player feedback on being hurt
					playSoundEntity(this, 28, 64); // "Damage.ogg"

					if ( myStats->HP > 0 )
					{
						if ( playerAutomaton )
						{
							messagePlayer(player, MESSAGE_STATUS, Language::get(3714));
						}
						else
						{
							messagePlayer(player, MESSAGE_STATUS, Language::get(633));
						}
					}

					// Shake the Host's screen
					if ( player >= 0 && players[player]->isLocalPlayer() )
					{
						camera_shakex += .1;
						camera_shakey += 10;
					}
					else if ( player > 0 && multiplayer == SERVER && !players[player]->isLocalPlayer() )
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

	if ( myStats->mask && myStats->mask->type == MASK_PIPE )
	{
		if ( !(behavior == &actPlayer && effectShapeshift != NOTHING)
			&& ((getHealthRegenInterval(this, *myStats, behavior == &actPlayer) == -1)
				|| myStats->type == INSECTOID
				|| (behavior == &actPlayer && myStats->playerRace == RACE_INSECTOID && myStats->stat_appearance == 0)) )
		{
			if ( ticks % (HEAL_TIME) == 0 )
			{
				steamAchievementEntity(this, "BARONY_ACH_SMOKIN");

				int damage = 1 + local_rng.rand() % 3;
				this->modHP(-damage);
				if ( myStats->HP <= 0 )
				{
					this->setObituary(Language::get(1534)); // choked to death
					myStats->killer = KilledBy::ITEM;
					myStats->killer_item = MASK_PIPE;
				}

				// Give the Player feedback on being hurt
				playSoundEntity(this, 28, 32); // "Damage.ogg"

				if ( myStats->HP > 0 )
				{
					messagePlayer(player, MESSAGE_STATUS, Language::get(6091));

					// Shake the Host's screen
					if ( myStats->HP <= 10 )
					{
						if ( player >= 0 && players[player]->isLocalPlayer() )
						{
							camera_shakex += .1;
							camera_shakey += 10;
						}
						else if ( player > 0 && multiplayer == SERVER && !players[player]->isLocalPlayer() )
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
					else
					{
						if ( player >= 0 && players[player]->isLocalPlayer() )
						{
							camera_shakex += .04;
							camera_shakey += 5;
						}
						else if ( player > 0 && multiplayer == SERVER && !players[player]->isLocalPlayer() )
						{
							// Shake the Client's screen
							strcpy((char*)net_packet->data, "SHAK");
							net_packet->data[4] = 4; // turns into .1
							net_packet->data[5] = 5;
							net_packet->address.host = net_clients[player - 1].host;
							net_packet->address.port = net_clients[player - 1].port;
							net_packet->len = 6;
							sendPacketSafe(net_sock, -1, net_packet, player - 1);
						}
					}
				}
			}
		}
	}

	// "random" vomiting
	if ( !this->char_gonnavomit && !myStats->EFFECTS[EFF_VOMITING] 
		&& this->entityCanVomit() )
	{
		if ( myStats->HUNGER > 1500 && local_rng.rand() % 1000 == 0 )
		{
			// oversatiation
			if ( !(svFlags & SV_FLAG_HUNGER) || MFLAG_DISABLEHUNGER )
			{
				myStats->HUNGER = std::min(myStats->HUNGER, 1000); // reset hunger to safe level.
			}
			else
			{
				messagePlayer(player, MESSAGE_STATUS, Language::get(634));
				this->char_gonnavomit = 140 + local_rng.rand() % 60;
			}
		}
		else if ( ticks % 60 == 0 && local_rng.rand() % 200 == 0 && myStats->EFFECTS[EFF_DRUNK] && myStats->type != GOATMAN )
		{
			// drunkenness
			messagePlayer(player, MESSAGE_STATUS, Language::get(634));
			this->char_gonnavomit = 140 + local_rng.rand() % 60;
		}
	}
	if ( this->char_gonnavomit > 0 )
	{
		this->char_gonnavomit--;
		if ( this->char_gonnavomit == 0 && this->entityCanVomit() )
		{
			messagePlayer(player, MESSAGE_STATUS, Language::get(635));
			myStats->EFFECTS[EFF_VOMITING] = true;
			myStats->EFFECTS_TIMERS[EFF_VOMITING] = 50 + local_rng.rand() % 20;
			serverUpdateEffects(player);
			if ( player >= 0 && players[player]->isLocalPlayer() )
			{
				camera_shakey += 9;
			}
			else if ( player > 0 && multiplayer == SERVER && !players[player]->isLocalPlayer() )
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

			if ( myStats->type == INSECTOID )
			{
				castSpell(uid, &spell_acidSpray, true, false);
			}
		}
	}

	// vomiting
	if ( myStats->EFFECTS[EFF_VOMITING] && ticks % 2 == 0 )
	{
		Entity* entity = spawnGib(this);
		if ( entity )
		{
			entity->sprite = 29;
            entity->ditheringDisabled = true;
			entity->flags[SPRITE] = true;
			entity->flags[GENIUS] = true;
			entity->flags[INVISIBLE] = false;
			entity->yaw = this->yaw - 0.1 + (local_rng.rand() % 20) * 0.01;
			entity->pitch = (local_rng.rand() % 360) * PI / 180.0;
			entity->roll = (local_rng.rand() % 360) * PI / 180.0;
			double vel = (local_rng.rand() % 15) / 10.f;
			entity->vel_x = vel * cos(entity->yaw);
			entity->vel_y = vel * sin(entity->yaw);
			entity->vel_z = -.5;
			if ( (svFlags & SV_FLAG_HUNGER) )
			{
				if ( myStats->type != INSECTOID && myStats->type != AUTOMATON
					&& myStats->type != SKELETON && effectShapeshift == NOTHING )
				{
					myStats->HUNGER -= 40;
					if ( myStats->HUNGER <= 50 )
					{
						myStats->HUNGER = 50;
						myStats->EFFECTS_TIMERS[EFF_VOMITING] = 1;
					}
				}
			}
			serverSpawnGibForClient(entity);
		}
	}

	int hpMod = 1;
	int mpMod = 1;
	if ( myStats->mask && myStats->mask->type == MASK_PIPE )
	{
		if ( !(behavior == &actPlayer && effectShapeshift != NOTHING)
			&& !(behavior == &actPlayer && myStats->playerRace == RACE_INSECTOID && myStats->stat_appearance == 0) )
		{
			if ( myStats->mask->beatitude >= 0 || shouldInvertEquipmentBeatitude(myStats) )
			{
				int chance = std::min(25 + (10 * abs(myStats->mask->beatitude)), 50);
				if ( local_rng.rand() % 100 < chance )
				{
					mpMod += 1;
					hpMod -= 1;
				}
			}
			else
			{
				int chance = std::min(25 + (10 * abs(myStats->mask->beatitude)), 50);
				if ( local_rng.rand() % 100 < chance )
				{
					mpMod -= 1;
					hpMod -= 1;
				}
			}
		}
	}
	if ( myStats->helmet && myStats->helmet->type == HAT_HOOD_APPRENTICE )
	{
		if ( myStats->helmet->beatitude >= 0 || shouldInvertEquipmentBeatitude(myStats) )
		{
			int chance = std::min(30 + (10 * abs(myStats->helmet->beatitude)), 50);
			if ( local_rng.rand() % 100 < chance )
			{
				mpMod += 1;
			}
		}
		else
		{
			int chance = std::min(30 + (10 * abs(myStats->helmet->beatitude)), 50);
			if ( local_rng.rand() % 100 < chance )
			{
				mpMod -= 1;
			}
		}
	}
	if ( int bonusFollowerRegen = getFollowerBonusHPRegen() )
	{
		hpMod += abs(2 * bonusFollowerRegen);
	}

	// healing over time
	int healring = 0;
	int healthRegenInterval = getHealthRegenInterval(this, *myStats, behavior == &actPlayer);
	bool naturalHeal = false;
	if ( healthRegenInterval >= 0 )
	{
		if ( myStats->HP < myStats->MAXHP )
		{
			this->char_heal++;
			/*if ( (svFlags & SV_FLAG_HUNGER) || behavior == &actMonster || (behavior == &actPlayer && myStats->type == SKELETON) )*/
			{
				if ( this->char_heal >= healthRegenInterval )
				{
					this->char_heal = 0;
					if ( hpMod > 0 )
					{
						Sint32 oldHP = myStats->HP;
						this->modHP(hpMod);
						naturalHeal = true;
						if ( behavior == &actPlayer )
						{
							if ( oldHP < myStats->HP )
							{
								Compendium_t::Events_t::eventUpdateCodex(skill[2], Compendium_t::CPDM_RGN_HP_RUN, "rgn", myStats->HP - oldHP);
								Compendium_t::Events_t::eventUpdateCodex(skill[2], Compendium_t::CPDM_RGN_HP_SUM, "rgn", myStats->HP - oldHP);
							}
							Compendium_t::Events_t::eventUpdateCodex(skill[2], Compendium_t::CPDM_RGN_HP_RATE_MAX, "rgn", healthRegenInterval);
						}
					}
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
			if ( local_rng.rand() % 1000 == 0 )   // .1% chance every frame
			{
				teleportRandom();
			}
		}
	}

	// regaining energy over time
	if ( myStats->type == AUTOMATON && player >= 0 )
	{
		int manaRegenInterval = Entity::getManaRegenInterval(this, *myStats, behavior == &actPlayer);
		this->char_energize++;

		if ( myStats->HUNGER <= 300 )
		{
			manaRegenInterval /= 6; // degrade faster
		}
		else if ( myStats->HUNGER > 1200 )
		{
			achievementObserver.playerAchievements[player].ticksSpentOverclocked++;
			if ( myStats->MP / static_cast<real_t>(std::max(1, myStats->MAXMP)) <= 0.5 )
			{
				manaRegenInterval /= 4; // increase faster at < 50% mana
			}
			else
			{
				manaRegenInterval /= 2; // increase less faster at > 50% mana
			}
		}
		else if ( myStats->HUNGER > 300 )
		{
			// normal manaRegenInterval 300-1200 hunger.
		}

		if ( this->char_energize >= manaRegenInterval && myStats->HUNGER <= 300 )
		{
			/*if ( local_rng.rand() % 5 == 0 )
			{
				messagePlayer(0, "1 MP every %f seconds", manaRegenInterval / 50.f);
			}*/
			this->char_energize = 0;
			if ( manaRegenInterval / 50.f < 0.5 ) // less than half a second, don't update clients as often.
			{
				if ( ticks % 25 == 0 )
				{
					this->modMP(-1, true);
				}
				else
				{
					this->modMP(-1, false);
				}
			}
			else
			{
				this->modMP(-1);
			}
		}
		else if ( this->char_energize >= manaRegenInterval )
		{
			/*if ( local_rng.rand() % 5 == 0 )
			{
				messagePlayer(0, "1 MP every %f seconds", manaRegenInterval / 50.f);
			}*/
			this->char_energize = 0;
			if ( mpMod > 0 )
			{
				Sint32 oldMP = myStats->MP;
				this->modMP(mpMod);
				if ( oldMP < myStats->MP )
				{
					if ( behavior == &actPlayer )
					{
						Compendium_t::Events_t::eventUpdateCodex(skill[2], Compendium_t::CPDM_RGN_MP_RUN, "rgn", myStats->MP - oldMP);
						Compendium_t::Events_t::eventUpdateCodex(skill[2], Compendium_t::CPDM_RGN_MP_SUM, "rgn", myStats->MP - oldMP);
					}
				}
			}
		}
	}
	else if ( this->behavior == &actPlayer && myStats->playerRace == RACE_INSECTOID && myStats->stat_appearance == 0 )
	{
		if ( (svFlags & SV_FLAG_HUNGER) && !MFLAG_DISABLEHUNGER )
		{
			this->char_energize++;
			if ( this->char_energize > 0 && this->char_energize % 5 == 0 ) // check every 5 ticks.
			{
				real_t manaPercentFromHunger = myStats->HUNGER / 1000.f;
				real_t expectedManaValue = std::floor(myStats->MAXMP * manaPercentFromHunger);
				Sint32 Sint32expectedMana = static_cast<Sint32>(expectedManaValue);
				if ( myStats->HUNGER > 0 ) 
				{
					// add extra expected mana point here.
					// i.e 950 hunger is still full mana to avoid always having 1 short.
					// skip 0 hunger as it will be 0 expected.
					Sint32expectedMana++;
				}
				//messagePlayer(0, "Hunger: %d, expected MP: %d", myStats->HUNGER, Sint32expectedMana);

				if ( myStats->MP < Sint32expectedMana )
				{
					if ( player == 0 ) // singleplayer/server only.
					{
						int difference = Sint32expectedMana - myStats->MP;
						if ( difference > 8 )
						{
							this->modMP(1);
							this->char_energize = 0;
						}
						else if ( difference > 4 )
						{
							if ( this->char_energize >= 10 )
							{
								this->modMP(1);
								this->char_energize = 0;
							}
						}
						else
						{
							if ( this->char_energize >= 15 )
							{
								this->modMP(1);
								this->char_energize = 0;
							}
						}
					}
					else
					{
						int difference = Sint32expectedMana - myStats->MP;
						if ( this->char_energize % 50 == 0 ) // only update clients every 1 second.
						{
							this->modMP(std::min(difference, 5)); // jump by max of 5.
							this->char_energize = 0;
						}
					}
				}
				else if ( myStats->MP > Sint32expectedMana )
				{
					if ( this->char_energize % 50 == 0 )
					{
						this->modMP(-1); // update MP decrease every second.
						this->char_energize = 0;
					}
				}
				else
				{
					this->char_energize = 0;
				}
			}
		}
	}
	else if ( myStats->MP < myStats->MAXMP )
	{
		int manaRegenInterval = Entity::getManaRegenInterval(this, *myStats, behavior == &actPlayer);
		// summons don't regen MP. we use this to refund mana to the caster.
		bool doManaRegen = true;
		if ( this->behavior == &actMonster && this->monsterAllySummonRank != 0 )
		{
			doManaRegen = false;
		}

		if ( doManaRegen )
		{
			this->char_energize++;
			if ( this->char_energize >= manaRegenInterval )
			{
				this->char_energize = 0;
				if ( mpMod > 0 )
				{
					Sint32 oldMP = myStats->MP;
					this->modMP(mpMod);
					if ( behavior == &actPlayer )
					{
						if ( oldMP < myStats->MP )
						{
							Compendium_t::Events_t::eventUpdateCodex(skill[2], Compendium_t::CPDM_RGN_MP_RUN, "rgn", myStats->MP - oldMP);
							Compendium_t::Events_t::eventUpdateCodex(skill[2], Compendium_t::CPDM_RGN_MP_SUM, "rgn", myStats->MP - oldMP);
						}
						Compendium_t::Events_t::eventUpdateCodex(skill[2], Compendium_t::CPDM_RGN_MP_RATE_MAX, "rgn", manaRegenInterval);
					}
				}
			}
		}
		else
		{
			this->char_energize = 0;
		}
	}
	else
	{
		this->char_energize = 0;
	}

	// effects of greasy fingers
	if ( myStats->EFFECTS[EFF_GREASY] == true )
	{
		// add some weird timing so it doesn't auto drop out of your hand immediately.
		// intended to fix multiplayer duplication.
		if ( ticks % 70 == 0 || ticks % 130 == 0 ) 
		{
			if ( behavior == &actMonster && myStats->HP > 0 )
			{
				if ( myStats->weapon != NULL && itemCategory(myStats->weapon) != SPELLBOOK )
				{
					//messagePlayer(player, MESSAGE_EQUIPMENT, Language::get(636));
					dropItemMonster(myStats->weapon, this, myStats);
				}
			}
		}
	}

	// torches/lamps burn down
	if ( myStats->shield != NULL )
	{
		if ( myStats->shield->type == TOOL_TORCH || myStats->shield->type == TOOL_LANTERN )
		{
			this->char_torchtime++;
			if ( (this->char_torchtime >= 7500 && myStats->shield->type == TOOL_TORCH) || (this->char_torchtime >= 10500) )
			{
				this->char_torchtime = 0;
				if ( behavior == &actPlayer && effectShapeshift != NOTHING )
				{
					// do nothing, shapeshifted
				}
				else if ( myStats->shield->type == TOOL_TORCH && player >= 0 )
				{
					std::string itemName = myStats->shield->getName();
					ItemType itemType = myStats->shield->type;
					Status itemStatus = myStats->shield->status;
					messagePlayer(player, MESSAGE_EQUIPMENT, Language::get(638), itemName.c_str());
					int qty = std::max(0, myStats->shield->count - 1);
					Item* item = myStats->shield;
					consumeItem(item, player);
					if ( qty > 0 && item )
					{
						messagePlayer(player, MESSAGE_EQUIPMENT, Language::get(4101), itemName.c_str()); // you reignite another torch
						playSoundEntity(this, 134, 64); // ignite
						if ( player >= 0 && players[player]->isLocalPlayer() )
						{
							players[player]->hud.shieldSwitch = true;
						}
					}
					if ( !item )
					{
						myStats->shield = nullptr;
					}
					if ( multiplayer == SERVER && player > 0 && !players[player]->isLocalPlayer() )
					{
						strcpy((char*)net_packet->data, "TORC");
						SDLNet_Write16((Sint16)itemType, &net_packet->data[4]);
						net_packet->data[6] = itemStatus;
						net_packet->data[7] = qty;
						net_packet->address.host = net_clients[player - 1].host;
						net_packet->address.port = net_clients[player - 1].port;
						net_packet->len = 8;
						sendPacketSafe(net_sock, -1, net_packet, player - 1);
					}
				}
				else // lanterns, monster torches
				{
					if ( player >= 0 && players[player]->isLocalPlayer() )
					{
						if ( myStats->shield->count > 1 )
						{
							Item* newTorch = newItem(myStats->shield->type, myStats->shield->status, myStats->shield->beatitude, myStats->shield->count - 1, myStats->shield->appearance, myStats->shield->identified, &myStats->inventory);
						}
					}
					myStats->shield->count = 1;
					myStats->shield->status = static_cast<Status>(myStats->shield->status - 1);
					if ( myStats->shield->status > BROKEN )
					{
						messagePlayer(player, MESSAGE_EQUIPMENT, Language::get(637), myStats->shield->getName());
					}
					else
					{
						messagePlayer(player, MESSAGE_EQUIPMENT, Language::get(638), myStats->shield->getName());
					}
					if ( multiplayer == SERVER && player > 0 && !players[player]->isLocalPlayer() )
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
	}

	// effects of being poisoned
	if ( myStats->EFFECTS[EFF_POISONED] )
	{
		if ( myStats->type == INSECTOID )
		{
			messagePlayer(player, MESSAGE_STATUS, Language::get(640));
			myStats->EFFECTS_TIMERS[EFF_POISONED] = 0;
			myStats->EFFECTS[EFF_POISONED] = false;
			serverUpdateEffects(player);
			this->char_poison = 0;
		}
		else if ( myStats->amulet && myStats->amulet->type == AMULET_POISONRESISTANCE )
		{
			messagePlayer(player, MESSAGE_EQUIPMENT | MESSAGE_HINT, Language::get(639));
			messagePlayer(player, MESSAGE_STATUS, Language::get(640));
			myStats->EFFECTS_TIMERS[EFF_POISONED] = 0;
			myStats->EFFECTS[EFF_POISONED] = false;
			serverUpdateEffects(player);
			this->char_poison = 0;
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
				poisonhurt -= local_rng.rand() % (std::max(1, poisonhurt / 4));
			}
			this->modHP(-poisonhurt);
			for ( int tmp = 0; tmp < 3; ++tmp )
			{
				Entity* gib = spawnGib(this, 211);
				serverSpawnGibForClient(gib);
			}
			Entity* killer = uidToEntity(myStats->poisonKiller);
			if ( myStats->HP <= 0 )
			{
				if ( killer )
				{
					killer->awardXP(this, true, true);
				}
				else
				{
					if ( achievementObserver.checkUidIsFromPlayer(myStats->poisonKiller) >= 0 )
					{
						steamAchievementClient(achievementObserver.checkUidIsFromPlayer(myStats->poisonKiller), "BARONY_ACH_TAKING_WITH");
					}
				}
			}
			if ( killer && killer->behavior == &actPlayer )
			{
				bool lowPriority = true;
				// update enemy bar for attacker
				if ( !strcmp(myStats->name, "") )
				{
					updateEnemyBar(killer, this, getMonsterLocalizedName(myStats->type).c_str(), myStats->HP, myStats->MAXHP, lowPriority,
						DamageGib::DMG_POISON);
				}
				else
				{
					updateEnemyBar(killer, this, myStats->name, myStats->HP, myStats->MAXHP, lowPriority,
						DamageGib::DMG_POISON);
				}
			}
			myStats->killer = KilledBy::POISON;
			this->setObituary(Language::get(1531));
			playSoundEntity(this, 28, 64);
			if ( player >= 0 && players[player]->isLocalPlayer() )
			{
				camera_shakex += .1;
				camera_shakey += 10;
			}
			else if ( player > 0 && multiplayer == SERVER && !players[player]->isLocalPlayer() )
			{
				strcpy((char*)net_packet->data, "SHAK");
				net_packet->data[4] = 10; // turns into .1
				net_packet->data[5] = 10;
				net_packet->address.host = net_clients[player - 1].host;
				net_packet->address.port = net_clients[player - 1].port;
				net_packet->len = 6;
				sendPacketSafe(net_sock, -1, net_packet, player - 1);
			}
			if ( local_rng.rand() % 5 == 0 && getCON() >= -3 )
			{
				if ( myStats->mask != nullptr && myStats->mask->type == MASK_PLAGUE
					&& !(myStats->type != HUMAN && effectShapeshift != NOTHING)
					&& !(myStats->mask->beatitude >= 0 || shouldInvertEquipmentBeatitude(myStats)) )
				{
					// don't cure wearing cursed plague mask
				}
				else
				{
					messagePlayer(player, MESSAGE_STATUS, Language::get(641));
					myStats->EFFECTS_TIMERS[EFF_POISONED] = 0;
					myStats->EFFECTS[EFF_POISONED] = false;
					serverUpdateEffects(player);
				}
			}
		}
	}
	else
	{
		this->char_poison = 0;
		myStats->poisonKiller = 0;
	}

	if ( !myStats->EFFECTS[EFF_WEBBED] )
	{
		if ( creatureWebbedSlowCount > 0 )
		{
			creatureWebbedSlowCount = 0; // reset counter.
			if ( behavior == &actPlayer )
			{
				serverUpdateEntitySkill(this, 52); // update player.
			}
		}
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
					bleedhurt -= local_rng.rand() % (std::max(1, bleedhurt / 2));
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
				this->setObituary(Language::get(1532));
				myStats->killer = KilledBy::BLEEDING;
				Entity* gib = spawnGib(this);
				serverSpawnGibForClient(gib);
				if ( player >= 0 && players[player]->isLocalPlayer() )
				{
					camera_shakex -= .03;
					camera_shakey += 3;
				}
				else if ( player > 0 && multiplayer == SERVER && !players[player]->isLocalPlayer() )
				{
					strcpy((char*)net_packet->data, "SHAK");
					net_packet->data[4] = -3; // turns into -.03
					net_packet->data[5] = 3;
					net_packet->address.host = net_clients[player - 1].host;
					net_packet->address.port = net_clients[player - 1].port;
					net_packet->len = 6;
					sendPacketSafe(net_sock, -1, net_packet, player - 1);
				}
				messagePlayer(player, MESSAGE_STATUS, Language::get(642));
				
				{
					Entity* entity = nullptr;
					if ( spawn_blood && gibtype[myStats->type] == 1 )
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
						entity->z = 8.0 + (local_rng.rand() % 20) / 100.0;
						entity->parent = this->uid;
						entity->sizex = 2;
						entity->sizey = 2;
						entity->yaw = (local_rng.rand() % 360) * PI / 180.0;
						entity->flags[UPDATENEEDED] = true;
						entity->flags[PASSABLE] = true;
					}
				}

				Entity* killer = uidToEntity(static_cast<Uint32>(myStats->bleedInflictedBy));
				if ( killer && killer->behavior == &actPlayer )
				{
					bool lowPriority = true;
					// update enemy bar for attacker
					if ( !strcmp(myStats->name, "") )
					{
						updateEnemyBar(killer, this, getMonsterLocalizedName(myStats->type).c_str(), myStats->HP, myStats->MAXHP, lowPriority,
							DamageGib::DMG_BLEED);
					}
					else
					{
						updateEnemyBar(killer, this, myStats->name, myStats->HP, myStats->MAXHP, lowPriority,
							DamageGib::DMG_BLEED);
					}
				}
			}
			else
			{
				messagePlayer(player, MESSAGE_STATUS, Language::get(643));
				myStats->EFFECTS[EFF_BLEEDING] = false;
				myStats->EFFECTS_TIMERS[EFF_BLEEDING] = 0;
				serverUpdateEffects(player);
			}
		}
	}
	else
	{
		myStats->bleedInflictedBy = 0;
	}
	
	// webbed
	if ( myStats->EFFECTS[EFF_WEBBED] )
	{
		if ( ticks % 25 == 0 )
		{
			Entity* gib = spawnGib(this, 863);
			serverSpawnGibForClient(gib);
		}
		if ( ticks % 40 == 0 )
		{
			Entity* entity = newEntity(862, 1, map.entities, nullptr); //Web pool entity.
			if ( entity != NULL )
			{
				entity->x = this->x;
				entity->y = this->y;
				entity->z = 8.0 + (local_rng.rand() % 20) / 100.0;
				entity->parent = this->uid;
				entity->sizex = 2;
				entity->sizey = 2;
				entity->yaw = (local_rng.rand() % 360) * PI / 180.0;
				real_t scale = 0.75 + 0.25 * (local_rng.rand() % 100) / 100.f;
				entity->scalex = scale;
				entity->scaley = scale;
				entity->flags[UPDATENEEDED] = true;
				entity->flags[PASSABLE] = true;
			}
		}
	}

	if ( player >= 0 && (myStats->EFFECTS[EFF_LEVITATING] || myStats->EFFECTS[EFF_FLUTTER]) && MFLAG_DISABLELEVITATION)
	{
		Uint32 color = makeColorRGB(255, 0, 255);
		messagePlayerColor(player, MESSAGE_HINT, color, Language::get(2382)); // disabled levitation.
		this->setEffect(EFF_LEVITATING, false, 0, true);
		this->setEffect(EFF_FLUTTER, false, 0, true);
	}

	if ( myStats->EFFECTS[EFF_MAGICREFLECT] )
	{
		spawnAmbientParticles(80, 579, 10 + local_rng.rand() % 40, 1.0, false);
	}

	if (myStats->EFFECTS[EFF_VAMPIRICAURA])
	{
		spawnAmbientParticles(40, 600, 20 + local_rng.rand() % 30, 0.5, true);
	}

	if ( myStats->EFFECTS[EFF_FEAR] )
	{
		if ( ticks % 25 == 0 || ticks % 40 == 0 )
		{
			spawnAmbientParticles(1, 864, 20 + local_rng.rand() % 10, 0.5, true);
		}
	}

	if ( myStats->EFFECTS[EFF_TROLLS_BLOOD] )
	{
		spawnAmbientParticles(80, 169, 20 + local_rng.rand() % 10, 0.5, true);
	}

	if ( myStats->EFFECTS[EFF_PACIFY] )
	{
		if ( ticks % 25 == 0 || ticks % 40 == 0 )
		{
			spawnAmbientParticles(1, 685, 20 + local_rng.rand() % 10, 0.5, true);
		}
	}
	else if ( myStats->monsterIsCharmed == 1 )
	{
		if ( ticks % 80 == 0 || ticks % 100 == 0 )
		{
			spawnAmbientParticles(1, 685, 20 + local_rng.rand() % 10, 0.5, true);
		}
	}

	if ( myStats->EFFECTS[EFF_SHADOW_TAGGED] )
	{
		if ( ticks % 25 == 0 || ticks % 40 == 0 )
		{
			spawnAmbientParticles(1, 871, 20 + local_rng.rand() % 10, 0.5, true);
		}
	}

	if ( myStats->EFFECTS[EFF_POLYMORPH] )
	{
		if ( ticks % 25 == 0 || ticks % 40 == 0 )
		{
			spawnAmbientParticles(1, 593, 20 + local_rng.rand() % 10, 0.5, true);
		}
	}

	if ( myStats->EFFECTS[EFF_INVISIBLE] && myStats->type == SHADOW )
	{
		spawnAmbientParticles(20, 175, 20 + local_rng.rand() % 30, 0.5, true);
	}

	//if ( myStats->EFFECTS[EFF_BLIND] )
	//{
	//	spawnAmbientParticles2(2, 175, 20, 0.5, true); // maybe some black clouds
	//}

	// Process Burning Status Effect
	if ( this->flags[BURNING] )
	{
		this->char_fire--; // Decrease the fire counter
		
		// Check to see if time has run out
		if ( this->char_fire <= 0 )
		{
			this->flags[BURNING] = false;
			messagePlayer(player, MESSAGE_STATUS, Language::get(647)); // "The flames go out."
			serverUpdateEntityFlag(this, BURNING);
		}
		else
		{
			// If 0.6 seconds have passed (30 ticks), process the Burning Status Effect
			if ( (this->char_fire % TICKS_TO_PROCESS_FIRE) == 0 )
			{
				bool warmHat = false;

				// Buddha should not die to fire
				if ( buddhamode )
				{
					Sint32 fireDamage = (-2 - local_rng.rand() % 3); // Deal between -2 to -5 damage

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
					int damage = -2 - local_rng.rand() % 4; // Deal between -2 to -5 damage

					real_t fireMultiplier = 1.0;
					if ( myStats->helmet && myStats->helmet->type == HAT_WARM && local_rng.rand() % 4 == 0 )
					{
						if ( !(behavior == &actPlayer && effectShapeshift != NOTHING) )
						{
							if ( myStats->helmet->beatitude >= 0 || shouldInvertEquipmentBeatitude(myStats) )
							{
								//fireMultiplier += 1.0;
							}
							else
							{
								fireMultiplier += 0.5 + 0.5 * abs(myStats->helmet->beatitude); // cursed, extra fire damage
							}
							warmHat = true;
						}
					}
					damage *= fireMultiplier;

					this->modHP(damage); // Deal between -2 to -5 damage

					Entity* killer = uidToEntity(static_cast<Uint32>(myStats->burningInflictedBy));
					// If the Entity died, handle experience
					if ( myStats->HP <= 0 )
					{
						this->setObituary(Language::get(1533)); // "burns to a crisp."
				        myStats->killer = KilledBy::BURNING_TO_CRISP;

						if ( killer != nullptr )
						{
							killer->awardXP(this, true, true);
						}
						else 
						{
							if ( achievementObserver.checkUidIsFromPlayer(static_cast<Uint32>(myStats->burningInflictedBy)) >= 0 )
							{
								steamAchievementClient(achievementObserver.checkUidIsFromPlayer(myStats->burningInflictedBy), "BARONY_ACH_TAKING_WITH");
							}
						}
					}

					if ( killer && killer->behavior == &actPlayer )
					{
						bool lowPriority = true;
						// update enemy bar for attacker
						if ( !strcmp(myStats->name, "") )
						{
							updateEnemyBar(killer, this, getMonsterLocalizedName(myStats->type).c_str(), myStats->HP, myStats->MAXHP, lowPriority,
								DamageGib::DMG_FIRE);
						}
						else
						{
							updateEnemyBar(killer, this, myStats->name, myStats->HP, myStats->MAXHP, lowPriority,
								DamageGib::DMG_FIRE);
						}
					}
				}

				// Give the Player feedback on being hurt
				if ( warmHat )
				{
					messagePlayer(player, MESSAGE_STATUS, Language::get(6092)); // hat too hot
				}
				else
				{
					messagePlayer(player, MESSAGE_SPAM_MISC, Language::get(644)); // "It burns! It burns!"
				}
				playSoundEntity(this, 28, 64); // "Damage.ogg"

				// Shake the Camera
				if ( player >= 0 && players[player]->isLocalPlayer() )
				{
					camera_shakey += 5;
				}
				else if ( player > 0 && multiplayer == SERVER && !players[player]->isLocalPlayer() )
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
					if ( local_rng.rand() % 10 == 0 && myStats->cloak->type != ARTIFACT_CLOAK && myStats->cloak->type != CLOAK_BACKPACK )
					{
						if ( player >= 0 && players[player]->isLocalPlayer() )
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
							messagePlayer(player, MESSAGE_EQUIPMENT, Language::get(645), myStats->cloak->getName()); // "Your %s smoulders!"
						}
						else
						{
							messagePlayer(player, MESSAGE_EQUIPMENT, Language::get(646), myStats->cloak->getName()); // "Your %s burns to ash!"
							//Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_CLOAK_BURNED, myStats->cloak->type, 1);
							Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_BROKEN, myStats->cloak->type, 1);
						}
						if ( player > 0 && multiplayer == SERVER && !players[player]->isLocalPlayer() )
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
				if ( (local_rng.rand() % this->chanceToPutOutFire) == 0 )
				{
					this->flags[BURNING] = false;
					messagePlayer(player, MESSAGE_STATUS, Language::get(647)); // "The flames go out."
					serverUpdateEntityFlag(this, BURNING);
				}
			}
		}
	}
	else
	{
		this->char_fire = 0; // If not on fire, then reset fire counter TODOR: This seems unecessary, but is what poison does, this is happening every tick
		myStats->burningInflictedBy = 0;
	}

	if ( player >= 0 && (stats[player]->type == SKELETON || (stats[player]->playerRace == RACE_SKELETON && stats[player]->stat_appearance == 0)) )
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
				Entity* mySummon = nullptr;
				if ( c )
				{
					mySummon = uidToEntity(*c);
				}
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
				messagePlayer(player, MESSAGE_STATUS, Language::get(651));
				if ( revivedWithFriendship )
				{
					messagePlayer(player, MESSAGE_STATUS, Language::get(3198));
				}
				else
				{
					messagePlayer(player, MESSAGE_STATUS, Language::get(3180));
				}
				messagePlayer(player, MESSAGE_STATUS, Language::get(654));

				steamAchievementClient(player, "BARONY_ACH_SECOND_CHANCE");

				playSoundEntity(this, 167, 128);
				createParticleDropRising(this, 174, 1.0);
				serverSpawnMiscParticles(this, PARTICLE_EFFECT_RISING_DROP, 174);
				// convert MP to HP
				manaTotal = myStats->MP;
				if ( safeConsumeMP(myStats->MP) )
				{
					this->setHP(std::min(manaTotal, myStats->MAXHP));
					if ( player > 0 && multiplayer == SERVER && !players[player]->isLocalPlayer() )
					{
						strcpy((char*)net_packet->data, "ATTR");
						net_packet->data[4] = clientnum;
						net_packet->data[5] = (Sint8)myStats->STR;
						net_packet->data[6] = (Sint8)myStats->DEX;
						net_packet->data[7] = (Sint8)myStats->CON;
						net_packet->data[8] = (Sint8)myStats->INT;
						net_packet->data[9] = (Sint8)myStats->PER;
						net_packet->data[10] = (Sint8)myStats->CHR;
						net_packet->data[11] = (Uint8)myStats->EXP;
						net_packet->data[12] = (Uint8)myStats->LVL;
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
					if ( !(c == EFF_VAMPIRICAURA && myStats->EFFECTS_TIMERS[c] == -2) 
						&& c != EFF_WITHDRAWAL && c != EFF_SHAPESHIFT )
					{
						myStats->EFFECTS[c] = false;
						myStats->EFFECTS_TIMERS[c] = 0;
					}
				}

				myStats->EFFECTS[EFF_LEVITATING] = true;
				myStats->EFFECTS_TIMERS[EFF_LEVITATING] = 5 * TICKS_PER_SECOND;

				this->flags[BURNING] = false;
				serverUpdateEntityFlag(this, BURNING);
				serverUpdateEffects(player);
			}
			else
			{
				messagePlayer(player, MESSAGE_HINT, Language::get(3181));
			}
		}
	}

	// amulet effects
	if ( myStats->amulet != NULL )
	{
		// strangulation
		if ( myStats->amulet->type == AMULET_STRANGULATION && myStats->type != SKELETON )
		{
			if ( ticks % 60 == 0 )
			{
				if ( local_rng.rand() % 25 )
				{
					messagePlayer(player, MESSAGE_STATUS, Language::get(648));
					this->modHP(-(2 + local_rng.rand() % 3));
					playSoundEntity(this, 28, 64); // "Damage.ogg"
					if ( player >= 0 )
					{
						if ( myStats->type == SUCCUBUS || myStats->type == INCUBUS )
						{
							if ( local_rng.rand() % 3 > 0 && myStats->MP < myStats->MAXMP )
							{
								Uint32 color = makeColorRGB(0, 255, 0);
								messagePlayerColor(player, MESSAGE_HINT, color, Language::get(3358));
								int amount = 2 + local_rng.rand() % 2;
								int oldMP = myStats->MP;
								this->modMP(amount);
								if ( player >= 0 && stats[player]->stat_appearance == 0 )
								{
									if ( stats[player]->playerRace == RACE_INCUBUS || stats[player]->playerRace == RACE_SUCCUBUS )
									{
										if ( oldMP < myStats->MP )
										{
											steamStatisticUpdateClient(player, STEAM_STAT_SERIAL_THRILLA, STEAM_STAT_INT, myStats->MP - oldMP);
										}
									}
								}
							}
						}
					}
					this->setObituary(Language::get(1534)); // choked to death
			        myStats->killer = KilledBy::STRANGULATION;
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
						if ( player > 0 && multiplayer == SERVER && !players[player]->isLocalPlayer() )
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
					if ( player >= 0 && players[player]->isLocalPlayer() )
					{
						camera_shakey += 8;
					}
					else if ( player > 0 && multiplayer == SERVER && !players[player]->isLocalPlayer() )
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
					messagePlayer(player, MESSAGE_STATUS | MESSAGE_EQUIPMENT, Language::get(649));
					messagePlayer(player, MESSAGE_STATUS | MESSAGE_EQUIPMENT, Language::get(650));
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
					Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_BROKEN, myStats->amulet->type, 1);
					playSoundEntity(this, 76, 64);
					if ( player > 0 && multiplayer == SERVER && !players[player]->isLocalPlayer() )
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
					messagePlayer(player, MESSAGE_STATUS | MESSAGE_OBITUARY, Language::get(651));
				}
				if ( !this->isBlind() )
				{
					messagePlayer(player, MESSAGE_STATUS | MESSAGE_OBITUARY, Language::get(652));
				}
				else
				{
					messagePlayer(player, MESSAGE_STATUS | MESSAGE_OBITUARY, Language::get(653));
				}
				if ( myStats->amulet->beatitude >= 0 || shouldInvertEquipmentBeatitude(myStats) )
				{
					messagePlayer(player, MESSAGE_STATUS | MESSAGE_OBITUARY, Language::get(654));
					messagePlayer(player, MESSAGE_STATUS | MESSAGE_OBITUARY, Language::get(655));

					playSoundEntity(this, 167, 128);
					createParticleDropRising(this, 174, 1.0);
					serverSpawnMiscParticles(this, PARTICLE_EFFECT_RISING_DROP, 174);

					steamAchievementClient(player, "BARONY_ACH_BORN_AGAIN");
					myStats->HUNGER = 800;
					if ( myStats->MAXHP < 10 )
					{
						myStats->MAXHP = 10;
						if ( player > 0 && multiplayer == SERVER && !players[player]->isLocalPlayer() )
						{
							strcpy((char*)net_packet->data, "ATTR");
							net_packet->data[4] = clientnum;
							net_packet->data[5] = (Sint8)myStats->STR;
							net_packet->data[6] = (Sint8)myStats->DEX;
							net_packet->data[7] = (Sint8)myStats->CON;
							net_packet->data[8] = (Sint8)myStats->INT;
							net_packet->data[9] = (Sint8)myStats->PER;
							net_packet->data[10] = (Sint8)myStats->CHR;
							net_packet->data[11] = (Uint8)myStats->EXP;
							net_packet->data[12] = (Uint8)myStats->LVL;
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
						if ( !(c == EFF_VAMPIRICAURA && myStats->EFFECTS_TIMERS[c] == -2) 
							&& c != EFF_WITHDRAWAL && c != EFF_SHAPESHIFT )
						{
							myStats->EFFECTS[c] = false;
							myStats->EFFECTS_TIMERS[c] = 0;
						}
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
					messagePlayer(player, MESSAGE_STATUS | MESSAGE_OBITUARY, Language::get(656));
					messagePlayer(player, MESSAGE_STATUS | MESSAGE_OBITUARY, Language::get(657));
				}
				myStats->amulet->status = BROKEN;
				Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_BROKEN, myStats->amulet->type, 1);
				playSoundEntity(this, 76, 64);
				if ( player > 0 && multiplayer == SERVER && !players[player]->isLocalPlayer() )
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

	if ( player >= 0 && myStats->mask != nullptr )
	{
		if ( myStats->mask->type == TOOL_BLINDFOLD_TELEPATHY
			&& (ticks % 45 == 0 || !myStats->EFFECTS[EFF_TELEPATH]) )
		{
			setEffect(EFF_TELEPATH, true, 60, true);
		}
		else if ( myStats->mask->type == MASK_PLAGUE && !(myStats->type != HUMAN && effectShapeshift != NOTHING) )
		{
			if ( ticks % 45 == 0 || !myStats->EFFECTS[EFF_NAUSEA_PROTECTION] )
			{
				setEffect(EFF_NAUSEA_PROTECTION, true, 60, true);
			}
			if ( ticks % 45 == 0 || !myStats->EFFECTS[EFF_POISONED] )
			{
				if ( !(myStats->type == INSECTOID || (myStats->amulet && myStats->amulet->type == AMULET_POISONRESISTANCE)) )
				{
					bool cursedItemIsBuff = shouldInvertEquipmentBeatitude(myStats);
					if ( myStats->mask->beatitude >= 0 || cursedItemIsBuff )
					{
						// nothing, good
					}
					else
					{
						bool poisoned = myStats->EFFECTS[EFF_POISONED];
						setEffect(EFF_POISONED, true, (TICKS_PER_SECOND + 1) * 3, true);
						if ( !poisoned && myStats->EFFECTS[EFF_POISONED] )
						{
							myStats->poisonKiller = 0;
							messagePlayerColor(player, MESSAGE_STATUS, makeColorRGB(255, 0, 0), 
								Language::get(6086), myStats->mask->getName());
						}
					}
				}
			}
		}
	}

	bool freeAction = false;
	if ( player >= 0
		&& myStats->mask != nullptr
		&& (myStats->mask->type == TOOL_BLINDFOLD || myStats->mask->type == TOOL_BLINDFOLD_FOCUS || myStats->mask->type == TOOL_BLINDFOLD_TELEPATHY )
		&& (ticks % 45 == 0 || !myStats->EFFECTS[EFF_BLIND]) )
	{
		setEffect(EFF_BLIND, true, 60, true);
		if ( myStats->mask->type == TOOL_BLINDFOLD_FOCUS )
		{
			freeAction = true;
		}
	}

	if ( ticks % 45 == 0 && myStats->type == GOATMAN && myStats->EFFECTS[EFF_DRUNK] )
	{
		freeAction = true;
	}

	if ( freeAction )
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
		if ( myStats->EFFECTS_TIMERS[EFF_SLOW] > 0 )
		{
			cured = true;
			myStats->EFFECTS_TIMERS[EFF_SLOW] = 1; // tick over to 0 and dissipate on the next check, and play the appropriate message.
		}
		if ( myStats->EFFECTS_TIMERS[EFF_WEBBED] > 0 )
		{
			cured = true;
			myStats->EFFECTS_TIMERS[EFF_WEBBED] = 1; // tick over to 0 and dissipate on the next check, and play the appropriate message.
		}
		if ( cured )
		{
			playSoundEntity(this, 168, 128);
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
	if ( myStats->EFFECTS[EFF_ASLEEP] && (myStats->OLDHP > myStats->HP || (myStats->type >= LICH && myStats->type < KOBOLD)
		|| myStats->type == COCKATRICE || myStats->type == LICH_FIRE || myStats->type == LICH_ICE) )
	{
		messagePlayer(player, MESSAGE_STATUS, Language::get(658));
		if ( monsterAllyGetPlayerLeader() && monsterAllySpecial == ALLY_SPECIAL_CMD_REST )
		{
			// allies resting. if poison/bleed damage here, then ignore it (startingHPInHandleEffects will equal current HP)
			if ( !naturalHeal && startingHPInHandleEffects == myStats->HP )
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
	else if ( myStats->EFFECTS[EFF_ASLEEP] && monsterAllyGetPlayerLeader() && monsterAllySpecial == ALLY_SPECIAL_CMD_REST )
	{
		if ( myStats->HP == myStats->MAXHP )
		{
			myStats->EFFECTS[EFF_ASLEEP] = false; // wake up
			myStats->EFFECTS_TIMERS[EFF_ASLEEP] = 0;
			myStats->EFFECTS[EFF_HP_REGEN] = false; // stop regen
			myStats->EFFECTS_TIMERS[EFF_HP_REGEN] = 0;
			monsterAllySpecial = ALLY_SPECIAL_CMD_NONE;
			messagePlayerMonsterEvent(monsterAllyIndex, 0xFFFFFFFF, *myStats, Language::get(3881), Language::get(3881), MSG_GENERIC);
		}
	}
	myStats->OLDHP = myStats->HP;
}

real_t Entity::getACEffectiveness(Entity* my, Stat* myStats, bool isPlayer, Entity* attacker, Stat* attackerStats, int& outNumBlessings)
{
	if ( !myStats || !my )
	{
		return 1.0;
	}

	if ( myStats->defending )
	{
		return 1.0;
	}

	int blessings = 0;
	bool cursedItemIsBuff = shouldInvertEquipmentBeatitude(myStats);

	if ( myStats->helmet && Item::doesItemProvideBeatitudeAC(myStats->helmet->type) )
	{
		blessings += cursedItemIsBuff ? abs(myStats->helmet->beatitude) : myStats->helmet->beatitude;
	}
	if ( myStats->breastplate && Item::doesItemProvideBeatitudeAC(myStats->breastplate->type) )
	{
		blessings += cursedItemIsBuff ? abs(myStats->breastplate->beatitude) : myStats->breastplate->beatitude;
	}
	if ( myStats->gloves && Item::doesItemProvideBeatitudeAC(myStats->gloves->type) )
	{
		blessings += cursedItemIsBuff ? abs(myStats->gloves->beatitude) : myStats->gloves->beatitude;
	}
	if ( myStats->shoes && Item::doesItemProvideBeatitudeAC(myStats->shoes->type) )
	{
		blessings += cursedItemIsBuff ? abs(myStats->shoes->beatitude) : myStats->shoes->beatitude;
	}
	if ( myStats->shield && Item::doesItemProvideBeatitudeAC(myStats->shield->type) )
	{
		blessings += cursedItemIsBuff ? abs(myStats->shield->beatitude) : myStats->shield->beatitude;
	}
	if ( myStats->cloak && Item::doesItemProvideBeatitudeAC(myStats->cloak->type) )
	{
		blessings += cursedItemIsBuff ? abs(myStats->cloak->beatitude) : myStats->cloak->beatitude;
	}
	if ( myStats->ring && Item::doesItemProvideBeatitudeAC(myStats->ring->type) )
	{
		blessings += cursedItemIsBuff ? abs(myStats->ring->beatitude) : myStats->ring->beatitude;
	}
	if ( myStats->mask && Item::doesItemProvideBeatitudeAC(myStats->mask->type) )
	{
		blessings += cursedItemIsBuff ? abs(myStats->mask->beatitude) : myStats->mask->beatitude;
	}
	if ( myStats->amulet && Item::doesItemProvideBeatitudeAC(myStats->amulet->type) )
	{
		blessings += cursedItemIsBuff ? abs(myStats->amulet->beatitude) : myStats->amulet->beatitude;
	}
	outNumBlessings = blessings;
	return std::max(0.0, std::min(1.0, .75 + 0.025 * blessings));
}

/*-------------------------------------------------------------------------------

Entity::getAttack

returns the attack power of an entity based on strength, weapon, and a
base number

-------------------------------------------------------------------------------*/

Sint32 Entity::getAttack(Entity* my, Stat* myStats, bool isPlayer)
{
	Sint32 attack = 0;

	if ( !myStats )
	{
		return 0;
	}

	attack = BASE_MELEE_DAMAGE; // base attack strength
	bool shapeshifted = (my && my->behavior == &actPlayer && my->effectShapeshift != NOTHING);
	if ( myStats->weapon == nullptr || shapeshifted )
	{
		// bare handed.
		if ( isPlayer )
		{
			attack = BASE_PLAYER_UNARMED_DAMAGE;
			attack += (myStats->getModifiedProficiency(PRO_UNARMED) / 20); // 0, 1, 2, 3, 4, 5 damage from total
		}
		if ( myStats->gloves && !shapeshifted )
		{
			int beatitude = myStats->gloves->beatitude;
			if ( myStats->gloves->type == BRASS_KNUCKLES )
			{
				attack += 1 + (shouldInvertEquipmentBeatitude(myStats) ? abs(beatitude) : beatitude);
			}
			else if ( myStats->gloves->type == IRON_KNUCKLES )
			{
				attack += 2 + (shouldInvertEquipmentBeatitude(myStats) ? abs(beatitude) : beatitude);
			}
			else if ( myStats->gloves->type == SPIKED_GAUNTLETS )
			{
				attack += 3 + (shouldInvertEquipmentBeatitude(myStats) ? abs(beatitude) : beatitude);
			}
		}
		if ( myStats->ring )
		{
			int beatitude = myStats->ring->beatitude;
			attack += 1 + (shouldInvertEquipmentBeatitude(myStats) ? abs(beatitude) : beatitude);
		}
	}
	else if ( myStats->weapon != nullptr )
	{
		attack += myStats->weapon->weaponGetAttack(myStats);
	}

	if ( !shapeshifted && myStats->weapon && myStats->weapon->type == TOOL_WHIP )
	{
		int atk = statGetSTR(myStats, my) + statGetDEX(myStats, my);
		atk = std::min(atk / 2, atk);
		attack += atk;
	}
	else
	{
		attack += statGetSTR(myStats, my);
	}

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
		attack += entitystats->weapon->weaponGetAttack(entitystats);
		attack += getDEX();
		if ( behavior == &actMonster )
		{
			attack += getPER(); // monsters take PER into their ranged attacks to avoid having to increase their speed.
			attack += entitystats->getModifiedProficiency(PRO_RANGED) / 20; // 0 to 5 bonus attack for monsters
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

	int skillLVL = entitystats->getModifiedProficiency(PRO_RANGED) / 20;

	if ( entitystats->weapon )
	{
		if ( itemCategory(entitystats->weapon) == THROWN )
		{
			int dex = getDEX() / 4;
			attack += dex;
			attack += entitystats->weapon->weaponGetAttack(entitystats);
			attack *= thrownDamageSkillMultipliers[std::min(skillLVL, 5)];
		}
		else if ( itemCategory(entitystats->weapon) == POTION )
		{
			int skillLVL = entitystats->getModifiedProficiency(PRO_ALCHEMY) / 20;
			attack += entitystats->weapon->weaponGetAttack(entitystats);
			/*int dex = getDEX() / 4;
			attack += dex;*/
			attack *= potionDamageSkillMultipliers[std::min(skillLVL, 5)];
		}
		else
		{
			int dex = getDEX() / 4;
			attack += dex;
			attack += entitystats->weapon->weaponGetAttack(entitystats);
			attack += entitystats->getModifiedProficiency(PRO_RANGED) / 10; // 0 to 10 bonus attack.
		}
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
		if ( hitstats.EFFECTS[EFF_VAMPIRICAURA] )
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
	return statGetSTR(entitystats, this);
}

Sint32 statGetSTR(Stat* entitystats, Entity* my)
{
	Sint32 STR;

	if ( !entitystats )
	{
		return 0;
	}

	STR = entitystats->STR;

	bool cursedItemIsBuff = false;
	bool shapeshifted = false;
	if ( my && my->behavior == &actPlayer )
	{
		cursedItemIsBuff = shouldInvertEquipmentBeatitude(entitystats);
		if ( my->effectShapeshift != NOTHING )
		{
			shapeshifted = true;
			if ( my->effectShapeshift == TROLL )
			{
				int bonusSTR = 5;
				STR += bonusSTR;
				if ( STR >= 0 )
				{
					STR *= 1.33;
				}
			}
			else if ( my->effectShapeshift == SPIDER )
			{
				int bonusSTR = 3;
				STR += bonusSTR;
				if ( STR >= 0 )
				{
					STR *= 1.25;
				}
			}
		}
	}

	if ( svFlags & SV_FLAG_HUNGER )
	{
		if ( entitystats->type != AUTOMATON )
		{
			if ( entitystats->HUNGER >= getEntityHungerInterval(-1, my, entitystats, HUNGER_INTERVAL_OVERSATIATED) )
			{
				STR--;
			}
			if ( entitystats->HUNGER <= getEntityHungerInterval(-1, my, entitystats, HUNGER_INTERVAL_WEAK) )
			{
				STR--;
			}
			if ( entitystats->HUNGER <= getEntityHungerInterval(-1, my, entitystats, HUNGER_INTERVAL_STARVING) )
			{
				STR--;
			}
		}
		else if ( entitystats->type == AUTOMATON
			&& entitystats->HUNGER <= getEntityHungerInterval(-1, my, entitystats, HUNGER_INTERVAL_AUTOMATON_CRITICAL) )
		{
			STR--;
		}
	}
	if ( entitystats->gloves != nullptr )
	{
		if ( entitystats->gloves->type == GAUNTLETS_STRENGTH )
		{
			if ( entitystats->gloves->beatitude >= 0 || cursedItemIsBuff )
			{
				STR++;
			}
			STR += (cursedItemIsBuff ? abs(entitystats->gloves->beatitude) : entitystats->gloves->beatitude);
		}
	}
	if ( entitystats->helmet != nullptr )
	{
		if ( entitystats->helmet->type == HAT_WOLF_HOOD )
		{
			STR += (cursedItemIsBuff ? abs(entitystats->helmet->beatitude) : entitystats->helmet->beatitude);
		}
	}
	if ( entitystats->ring != nullptr )
	{
		if ( entitystats->ring->type == RING_STRENGTH )
		{
			if ( entitystats->ring->beatitude >= 0 || cursedItemIsBuff )
			{
				STR++;
			}
			STR += (cursedItemIsBuff ? abs(entitystats->ring->beatitude) : entitystats->ring->beatitude);
		}
	}
	if ( entitystats->EFFECTS[EFF_SHRINE_RED_BUFF] )
	{
		STR += 8;
	}
	if ( entitystats->EFFECTS[EFF_VAMPIRICAURA] && my && my->behavior == &actPlayer )
	{
		if ( entitystats->EFFECTS_TIMERS[EFF_VAMPIRICAURA] == -2 )
		{
			STR += 3; // player cursed vampiric bonus
		}
		else
		{
			STR += (std::max(5, STR / 4));
		}
	}
	if ( entitystats->EFFECTS[EFF_POTION_STR] )
	{
		STR += (std::max(5, STR / 4));
	}
	if ( entitystats->EFFECTS[EFF_DRUNK] )
	{
		switch ( entitystats->type )
		{
			case GOATMAN:
				if ( my && my->behavior == &actMonster )
				{
					STR += std::max(10, static_cast<int>(STR * 0.25)); //Goatman love booze.
				}
				else if ( my && my->behavior == &actPlayer )
				{
					STR += std::max(4, static_cast<int>(STR * 0.25));
				}
				break;
			default:
				++STR;
				break;
		}
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
	return statGetDEX(entitystats, this);
}

Sint32 statGetDEX(Stat* entitystats, Entity* my)
{
	Sint32 DEX;

	if ( !entitystats )
	{
		return 0;
	}

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

	bool cursedItemIsBuff = false;
	bool shapeshifted = false;
	if ( my && my->behavior == &actPlayer )
	{
		cursedItemIsBuff = shouldInvertEquipmentBeatitude(entitystats);
		if ( my->effectShapeshift != NOTHING )
		{
			shapeshifted = true;
			if ( my->effectShapeshift == TROLL )
			{
				int bonusDEX = -5;
				DEX += bonusDEX;
				if ( DEX >= 0 )
				{
					DEX *= 0.67;
				}
			}
			else if ( my->effectShapeshift == RAT )
			{
				int bonusDEX = 3;
				DEX += bonusDEX;
				if ( DEX >= 0 )
				{
					DEX *= 1.25;
				}
			}
		}
	}

	if ( entitystats->EFFECTS[EFF_VAMPIRICAURA] && !entitystats->EFFECTS[EFF_FAST] && !entitystats->EFFECTS[EFF_SLOW] )
	{
		if ( entitystats->EFFECTS_TIMERS[EFF_VAMPIRICAURA] == -2 )
		{
			DEX += 3; // player cursed vampiric bonus
		}
		else
		{
			DEX += 5;
			if ( my && entitystats->type == VAMPIRE && my->behavior == &actMonster )
			{
				DEX += 3; // monster vampires
			}
			if ( my && my->behavior == &actPlayer )
			{
				DEX += (std::max(0, DEX / 4));
			}
		}
	}
	else if ( entitystats->EFFECTS[EFF_FAST] && !entitystats->EFFECTS[EFF_SLOW] )
	{
		if ( my && my->behavior == &actPlayer )
		{
			DEX += 5;
		}
		else
		{
			DEX += 10;
		}
	}
	if ( entitystats->EFFECTS[EFF_STUNNED] )
	{
		//DEX -= 5;
	}

	if ( entitystats->EFFECTS[EFF_AGILITY] )
	{
		DEX += (std::max(5, DEX / 4));
	}

	if ( my && my->monsterAllyGetPlayerLeader() )
	{
		if ( stats[my->monsterAllyIndex] )
		{
			DEX += 1 + (stats[my->monsterAllyIndex]->getModifiedProficiency(PRO_LEADERSHIP) / 20);
		}
	}

	if ( my && my->behavior == &actPlayer && entitystats->type == AUTOMATON )
	{
		real_t ratio = entitystats->MP / static_cast<real_t>(entitystats->MAXMP);
		if ( ratio < 0.1 )
		{
			DEX -= std::max((std::max(0, DEX) / 2), 3);
		}
		else if ( ratio < 0.25 )
		{
			DEX -= std::max((std::max(0, DEX) / 4), 2);
		}

		if ( entitystats->HUNGER == 0 )
		{
			DEX -= 2;
		}
	}
	else if ( svFlags & SV_FLAG_HUNGER )
	{
		if ( entitystats->HUNGER >= getEntityHungerInterval(-1, my, entitystats, HUNGER_INTERVAL_OVERSATIATED) )
		{
			DEX--;
		}
		if ( entitystats->HUNGER <= getEntityHungerInterval(-1, my, entitystats, HUNGER_INTERVAL_WEAK) )
		{
			DEX--;
		}
		if ( entitystats->HUNGER <= getEntityHungerInterval(-1, my, entitystats, HUNGER_INTERVAL_STARVING) )
		{
			DEX--;
		}
	}

	if ( entitystats->EFFECTS[EFF_WEBBED] && !entitystats->EFFECTS[EFF_SLOW] )
	{
		DEX = std::max(std::min(DEX, 2) - 2 * (my ? my->creatureWebbedSlowCount : 0), -4);
	}
	if ( !entitystats->EFFECTS[EFF_FAST] && entitystats->EFFECTS[EFF_SLOW] )
	{
		if ( my && my->behavior == &actPlayer )
		{
			DEX = std::max(DEX - 5, -2);
		}
		else
		{
			DEX = std::min(DEX - 3, -2);
		}
	}
	if ( entitystats->shoes != nullptr )
	{
		if ( entitystats->shoes->type == LEATHER_BOOTS_SPEED )
		{
			if ( entitystats->shoes->beatitude >= 0 || cursedItemIsBuff )
			{
				DEX++;
			}
			DEX += (cursedItemIsBuff ? abs(entitystats->shoes->beatitude) : entitystats->shoes->beatitude);
		}
	}
	if ( entitystats->helmet != nullptr )
	{
		if ( entitystats->helmet->type == HAT_BUNNY_HOOD )
		{
			DEX += (cursedItemIsBuff ? abs(entitystats->helmet->beatitude) : entitystats->helmet->beatitude);
		}
		if ( entitystats->helmet->type == HAT_BYCOCKET )
		{
			DEX += (cursedItemIsBuff ? abs(entitystats->helmet->beatitude) : entitystats->helmet->beatitude);
		}
	}
	if ( entitystats->gloves != nullptr )
	{
		if ( entitystats->gloves->type == GLOVES_DEXTERITY )
		{
			if ( entitystats->gloves->beatitude >= 0 || cursedItemIsBuff )
			{
				DEX++;
			}
			DEX += (cursedItemIsBuff ? abs(entitystats->gloves->beatitude) : entitystats->gloves->beatitude);
		}
	}
	if ( entitystats->EFFECTS[EFF_DRUNK] )
	{
		switch ( entitystats->type )
		{
			case GOATMAN:
				DEX -= std::max(2, static_cast<int>(DEX * 0.25));
				break;
			default:
				--DEX;
				break;
		}
	}

	if ( !(svFlags & SV_FLAG_HUNGER) )
	{
		if ( my && my->behavior == &actPlayer && entitystats->playerRace == RACE_INSECTOID && entitystats->stat_appearance == 0 )
		{
			int dexDebuff = 0;
			if ( entitystats->MP < (entitystats->MAXMP) / 5 )
			{
				dexDebuff = 2;
			}
			else if ( entitystats->MP < 2 * (entitystats->MAXMP) / 5 )
			{
				dexDebuff = 1;
			}
			DEX -= dexDebuff;
			if ( DEX > 0 )
			{
				DEX -= dexDebuff * (DEX / 4); // -X DEX for every 4 DEX we have.
			}
		}
	}

	if ( entitystats->EFFECTS[EFF_WITHDRAWAL] && !entitystats->EFFECTS[EFF_DRUNK] )
	{
		DEX -= 3; // hungover.
		int minusDex = DEX;
		if ( minusDex > 0 )
		{
			DEX -= (minusDex / 4); // -1 DEX for every 4 DEX we have.
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
	return statGetCON(entitystats, this);
}

Sint32 statGetCON(Stat* entitystats, Entity* my)
{
	Sint32 CON;

	if ( !entitystats )
	{
		return 0;
	}

	CON = entitystats->CON;

	bool cursedItemIsBuff = false;
	bool shapeshifted = false;
	if ( my && my->behavior == &actPlayer )
	{
		cursedItemIsBuff = shouldInvertEquipmentBeatitude(entitystats);
		if ( my->effectShapeshift != NOTHING )
		{
			shapeshifted = true;
			if ( my->effectShapeshift == SPIDER )
			{
				int bonusCON = 3;
				CON += bonusCON;
				if ( CON >= 0 )
				{
					CON *= 1.25;
				}
			}
			else if ( my->effectShapeshift == TROLL )
			{
				int bonusCON = 5;
				CON += bonusCON;
				if ( CON >= 0 )
				{
					CON *= 1.33;
				}
			}
		}
	}

	if ( entitystats->ring != nullptr )
	{
		if ( entitystats->ring->type == RING_CONSTITUTION )
		{
			if ( entitystats->ring->beatitude >= 0 || cursedItemIsBuff )
			{
				CON++;
			}
			CON += (cursedItemIsBuff ? abs(entitystats->ring->beatitude) : entitystats->ring->beatitude);
		}
	}
	if ( entitystats->helmet != nullptr )
	{
		if ( entitystats->helmet->type == HAT_BEAR_HOOD )
		{
			CON += (cursedItemIsBuff ? abs(entitystats->helmet->beatitude) : entitystats->helmet->beatitude);
		}
	}
	if ( entitystats->gloves != nullptr )
	{
		if ( entitystats->gloves->type == BRACERS_CONSTITUTION )
		{
			if ( entitystats->gloves->beatitude >= 0 || cursedItemIsBuff )
			{
				CON++;
			}
			CON += (cursedItemIsBuff ? abs(entitystats->gloves->beatitude) : entitystats->gloves->beatitude);
		}
	}
	if ( entitystats->EFFECTS[EFF_SHRINE_RED_BUFF] )
	{
		CON += 8;
	}
	if ( my && entitystats->EFFECTS[EFF_DRUNK] && entitystats->type == GOATMAN )
	{
		CON += std::max(4, static_cast<int>(CON * 0.25));
	}
	if ( entitystats->EFFECTS[EFF_CON_BONUS] )
	{
		CON += 3;
		int percentHP = static_cast<int>(100.0 * (real_t)entitystats->HP / std::max(1, entitystats->MAXHP));
		percentHP = std::min(100, std::max(0, percentHP));
		percentHP = 100 - percentHP;
		CON += percentHP / 10;
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
	return statGetINT(entitystats, this);
}

Sint32 statGetINT(Stat* entitystats, Entity* my)
{
	Sint32 INT;

	if ( !entitystats )
	{
		return 0;
	}

	INT = entitystats->INT;

	bool cursedItemIsBuff = false;
	bool shapeshifted = false;
	if ( my && my->behavior == &actPlayer )
	{
		cursedItemIsBuff = shouldInvertEquipmentBeatitude(entitystats);
		if ( my->effectShapeshift != NOTHING )
		{
			shapeshifted = true;
			if ( my->effectShapeshift == RAT )
			{
				int bonusINT = 3;
				INT += bonusINT;
				if ( INT >= 0 )
				{
					INT *= 1.25;
				}
			}
			else if ( my->effectShapeshift == CREATURE_IMP )
			{
				int bonusINT = 5;
				INT += bonusINT;
				if ( INT >= 0 )
				{
					INT *= 1.33;
				}
			}
		}
	}

	if ( svFlags & SV_FLAG_HUNGER )
	{
		if ( entitystats->HUNGER <= getEntityHungerInterval(-1, my, entitystats, HUNGER_INTERVAL_STARVING) )
		{
			INT--;
		}
	}
	if ( entitystats->helmet != nullptr )
	{
		if ( entitystats->helmet->type == HAT_WIZARD )
		{
			if ( entitystats->helmet->beatitude >= 0 || cursedItemIsBuff )
			{
				INT++;
			}
			INT += (cursedItemIsBuff ? abs(entitystats->helmet->beatitude) : entitystats->helmet->beatitude);
		}
		else if ( entitystats->helmet->type == ARTIFACT_HELM )
		{
			if ( entitystats->helmet->beatitude >= 0 || cursedItemIsBuff )
			{
				INT += 8;
			}
			INT += (cursedItemIsBuff ? abs(entitystats->helmet->beatitude) : entitystats->helmet->beatitude);
		}
	}
	if ( entitystats->breastplate != nullptr )
	{
		if ( entitystats->breastplate->type == HEALER_DOUBLET || entitystats->breastplate->type == WIZARD_DOUBLET )
		{
			if ( entitystats->breastplate->beatitude >= 0 || cursedItemIsBuff )
			{
				INT++;
			}
			INT += (cursedItemIsBuff ? abs(entitystats->breastplate->beatitude) : entitystats->breastplate->beatitude);
		}
	}
	if ( entitystats->EFFECTS[EFF_SHRINE_BLUE_BUFF] )
	{
		INT += 8;
	}
	if ( my && entitystats->EFFECTS[EFF_DRUNK] && my->behavior == &actPlayer && entitystats->type == GOATMAN )
	{
		INT -= std::max(8, static_cast<int>(INT * 0.25));
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
	return statGetPER(entitystats, this);
}

Sint32 statGetPER(Stat* entitystats, Entity* my)
{
	Sint32 PER;

	if ( !entitystats )
	{
		return 0;
	}

	PER = entitystats->PER;

	bool cursedItemIsBuff = false;
	bool shapeshifted = false;
	if ( my && my->behavior == &actPlayer )
	{
		cursedItemIsBuff = shouldInvertEquipmentBeatitude(entitystats);
		if ( my->effectShapeshift != NOTHING )
		{
			shapeshifted = true;
			if ( my->effectShapeshift == SPIDER )
			{
				int bonusPER = 5;
				PER += bonusPER;
				if ( PER >= 0 )
				{
					PER *= 1.33;
				}
			}
			else if ( my->effectShapeshift == CREATURE_IMP )
			{
				int bonusPER = 3;
				PER += bonusPER;
				if ( PER >= 0 )
				{
					PER *= 1.25;
				}
			}
			else if ( my->effectShapeshift == RAT )
			{
				int bonusPER = 3;
				PER += bonusPER;
				if ( PER >= 0 )
				{
					PER *= 1.25;
				}
			}
		}
	}

	if ( svFlags & SV_FLAG_HUNGER )
	{
		if ( entitystats->type != AUTOMATON )
		{
			if ( entitystats->HUNGER <= getEntityHungerInterval(-1, my, entitystats, HUNGER_INTERVAL_STARVING) )
			{
				PER--;
			}
		}
		else if ( entitystats->type == AUTOMATON
			&& entitystats->HUNGER <= getEntityHungerInterval(-1, my, entitystats, HUNGER_INTERVAL_AUTOMATON_CRITICAL) )
		{
			PER--;
		}
	}
	if ( entitystats->mask )
	{
		if ( entitystats->mask->type == TOOL_GLASSES
			|| entitystats->mask->type == MONOCLE )
		{
			if ( entitystats->mask->beatitude >= 0 || cursedItemIsBuff )
			{
				PER++;
			}
			PER += (cursedItemIsBuff ? abs(entitystats->mask->beatitude) : entitystats->mask->beatitude);
		}
		else if ( entitystats->mask->type == TOOL_BLINDFOLD
					|| entitystats->mask->type == TOOL_BLINDFOLD_TELEPATHY
					|| entitystats->mask->type == TOOL_BLINDFOLD_FOCUS )
		{
			if ( entitystats->mask->type == TOOL_BLINDFOLD_TELEPATHY
				|| entitystats->mask->type == TOOL_BLINDFOLD_FOCUS )
			{
				PER += 0;
			}
			else
			{
				PER -= 10;
			}
			PER += (cursedItemIsBuff ? abs(entitystats->mask->beatitude) : entitystats->mask->beatitude);
		}
		else if ( entitystats->mask->type == MASK_HAZARD_GOGGLES
			|| entitystats->mask->type == MASK_TECH_GOGGLES )
		{
			PER += (cursedItemIsBuff ? abs(entitystats->mask->beatitude) : entitystats->mask->beatitude);
		}
	}
	if ( entitystats->breastplate )
	{
		if ( entitystats->breastplate->type == MACHINIST_APRON )
		{
			if ( entitystats->breastplate->beatitude >= 0 || cursedItemIsBuff )
			{
				PER += 2;
			}
			PER += (cursedItemIsBuff ? abs(entitystats->breastplate->beatitude) : entitystats->breastplate->beatitude);
		}
	}
	if ( entitystats->helmet )
	{
		if ( entitystats->helmet->type == HAT_STAG_HOOD )
		{
			PER += (cursedItemIsBuff ? abs(entitystats->helmet->beatitude) : entitystats->helmet->beatitude);
		}
	}

	if ( !(svFlags & SV_FLAG_HUNGER) )
	{
		if ( my && my->behavior == &actPlayer && entitystats->playerRace == RACE_INSECTOID && entitystats->stat_appearance == 0 )
		{
			int perDebuff = 0;
			if ( entitystats->MP < (entitystats->MAXMP) / 5 )
			{
				perDebuff = 2;
			}
			else if ( entitystats->MP < 2 * (entitystats->MAXMP) / 5 )
			{
				perDebuff = 1;
			}
			PER -= perDebuff;
			if ( PER > 0 )
			{
				PER -= perDebuff * (PER / 4); // -X DEX for every 4 DEX we have.
			}
		}
	}

	if ( entitystats->EFFECTS[EFF_SHRINE_GREEN_BUFF] )
	{
		PER += 8;
	}
	if ( entitystats->EFFECTS[EFF_POTION_STR] )
	{
		PER -= std::max(5, PER / 2);
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
	return statGetCHR(entitystats, this);
}

Sint32 statGetCHR(Stat* entitystats, Entity* my)
{
	Sint32 CHR;

	if ( !entitystats )
	{
		return 0;
	}

	CHR = entitystats->CHR;

	bool cursedItemIsBuff = false;
	bool shapeshifted = false;
	if ( my && my->behavior == &actPlayer )
	{
		cursedItemIsBuff = shouldInvertEquipmentBeatitude(entitystats);
		if ( my->effectShapeshift != NOTHING )
		{
			shapeshifted = true;
			//if ( my->effectShapeshift == CREATURE_IMP )
			//{
			//	int bonusCHR = (2 + (std::max(0, entitystats->CHR) / 10)); // +2 + 10% base CHR
			//	CHR += bonusCHR;
			//}
		}
	}

	if ( entitystats->helmet != nullptr )
	{
		if ( entitystats->helmet->type == HAT_JESTER || entitystats->helmet->type == HAT_SILKEN_BOW
			|| entitystats->helmet->type == HAT_PLUMED_CAP )
		{
			if ( entitystats->helmet->beatitude >= 0 || cursedItemIsBuff )
			{
				CHR++;
			}
			CHR += (cursedItemIsBuff ? abs(entitystats->helmet->beatitude) : entitystats->helmet->beatitude);
		}
	}
	if ( entitystats->mask != nullptr )
	{
		if ( entitystats->mask->type == MASK_MASQUERADE || entitystats->mask->type == MASK_MOUTH_ROSE )
		{
			if ( entitystats->mask->beatitude >= 0 || cursedItemIsBuff )
			{
				CHR++;
			}
			CHR += (cursedItemIsBuff ? abs(entitystats->mask->beatitude) : entitystats->mask->beatitude);
		}
	}
	if ( entitystats->ring != nullptr )
	{
		if ( entitystats->ring->type == RING_ADORNMENT )
		{
			if ( entitystats->ring->beatitude >= 0 || cursedItemIsBuff )
			{
				CHR++;
			}
			CHR += (cursedItemIsBuff ? abs(entitystats->ring->beatitude) : entitystats->ring->beatitude);
		}
	}
	if ( entitystats->monsterDemonHasBeenExorcised >= 3 )
	{
		CHR += 5;
	}
	if ( my && entitystats->EFFECTS[EFF_DRUNK] && my->behavior == &actPlayer && entitystats->type == GOATMAN )
	{
		CHR += std::max(4, static_cast<int>(CHR * .25));
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

	bool shapeshifted = false;
	if ( this->behavior == &actPlayer )
	{
		if ( effectShapeshift != NOTHING )
		{
			shapeshifted = true;
		}
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
	if ( entitystats->mask != nullptr && !shapeshifted )
	{
		if ( entitystats->mask->type == TOOL_BLINDFOLD
			|| entitystats->mask->type == TOOL_BLINDFOLD_TELEPATHY
			|| entitystats->mask->type == TOOL_BLINDFOLD_FOCUS )
		{
			return true;
		}
	}

	return false;
}

bool Entity::isWaterWalking() const
{
	if ( behavior == &actMonster )
	{
		if ( Stat* stats = getStats() )
		{
			if ( stats->shoes && stats->shoes->type == IRON_BOOTS_WATERWALKING )
			{
				return true;
			}
			if ( stats->type == SLIME )
			{
				if ( stats->getAttribute("slime_type") == "terrain_spawn_override" )
				{
					return true;
				}
				auto color = MonsterData_t::getKeyFromSprite(sprite, SLIME);
				if ( color == "slime blue"
					|| color == "slime tar" )
				{
					return true;
				}
			}
		}
	}
	return false;
}
bool Entity::isLavaWalking() const
{
	if ( behavior == &actMonster )
	{
		if ( Stat* stats = getStats() )
		{
			if ( stats->shoes && stats->shoes->type == IRON_BOOTS_WATERWALKING )
			{
				return true;
			}
			if ( stats->type == SLIME )
			{
				if ( stats->getAttribute("slime_type") == "terrain_spawn_override" )
				{
					return true;
				}
				auto color = MonsterData_t::getKeyFromSprite(sprite, SLIME);
				if ( color == "slime red" )
				{
					return true;
				}
			}
		}
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
	if ( intro ) 
	{
		// show up in hi-scores
		return false;
	}
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
	/*else if ( skillCapstoneUnlockedEntity(PRO_STEALTH) )
	{
		return true;
	}*/

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

	if ( behavior == &actPlayer && (entitystats->EFFECTS[EFF_PACIFY] || entitystats->EFFECTS[EFF_FEAR]) )
	{
		return false;
	}
	else if ( behavior == &actPlayer && entitystats->HP <= 0 )
	{
		return false;
	}

	if ( behavior == &actPlayer && 
		(this->skill[9] == MONSTER_POSE_SPECIAL_WINDUP1 || this->skill[9] == PLAYER_POSE_GOLEM_SMASH) // special strike attack
		)
	{
		return false;
	}

	if ( behavior == &actMonster && MainMenu::isCutsceneActive() )
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

	if ( isInertMimic() || (entitystats->type == MIMIC 
		&& (entitystats->EFFECTS[EFF_MIMIC_LOCKED] || monsterSpecialState == MIMIC_MAGIC)) )
	{
		return false;
	}

	if ( entitystats->type == BAT_SMALL && monsterSpecialState == BAT_REST )
	{
		return false;
	}

	if ( (entitystats->type == LICH_FIRE || entitystats->type == LICH_ICE)
		&& monsterLichBattleState < LICH_BATTLE_READY )
	{
		return false;
	}

	if ( entitystats->type == GYROBOT 
		&& (monsterSpecialState == GYRO_RETURN_LANDING 
			|| monsterSpecialState == GYRO_INTERACT_LANDING
			|| monsterSpecialState == GYRO_START_FLYING) )
	{
		return false;
	}
	else if ( (entitystats->type == DUMMYBOT || entitystats->type == SENTRYBOT || entitystats->type == SPELLBOT)
		&& (monsterSpecialState == DUMMYBOT_RETURN_FORM) )
	{
		return false;
	}
	
	if ( entitystats->MISC_FLAGS[STAT_FLAG_NPC] != 0 && !strcmp(entitystats->name, "scriptNPC") )
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
			players[player]->entity->skill[10] = 0; // PLAYER_ATTACKTIME
			if ( pose == MONSTER_POSE_SPECIAL_WINDUP1 || pose == PLAYER_POSE_GOLEM_SMASH || pose == MONSTER_POSE_SPECIAL_WINDUP2 )
			{
				players[player]->entity->skill[9] = pose; // PLAYER_ATTACK
				if ( pose == MONSTER_POSE_SPECIAL_WINDUP1 || pose == MONSTER_POSE_SPECIAL_WINDUP2 )
				{
					if ( multiplayer == SERVER )
					{
						if ( player >= 0 && player < MAXPLAYERS )
						{
							serverUpdateEntitySkill(players[player]->entity, 9);
							serverUpdateEntitySkill(players[player]->entity, 10);
						}
					}
					return;
				}
				else if ( pose == PLAYER_POSE_GOLEM_SMASH )
				{
					players[player]->entity->skill[10] = 1; // to avoid resetting the animation
				}
			}
			else if ( stats[player]->weapon != nullptr )
			{
				if ( stats[player]->type == CREATURE_IMP && itemCategory(stats[player]->weapon) != MAGICSTAFF )
				{
					players[player]->entity->skill[9] = 1;
				}
				else
				{
					players[player]->entity->skill[9] = pose; // PLAYER_ATTACK
				}
			}
			else
			{
				players[player]->entity->skill[9] = 1; // special case for punch to eliminate spanking motion :p
			}
		}
		else
		{
			if ( pose >= MONSTER_POSE_MELEE_WINDUP1 && pose <= MONSTER_POSE_SPECIAL_WINDUP3 )
			{
				// calls animation, but doesn't actually attack
				// this branch executes for most monsters
				monsterAttack = pose;
				if (myStats->type != SCARAB) {
				    monsterAttackTime = 0;
				}
				if ( multiplayer == SERVER )
				{
					// be sure to update the clients with the new wind-up pose.
					serverUpdateEntitySkill(this, 8);
					if (myStats->type != SLIME && myStats->type != RAT && myStats->type != SCARAB) {
					    serverUpdateEntitySkill(this, 9);
					}
				}
				return; // don't execute the attack, let the monster animation call the attack() function again.
			}
			else if ( (myStats->type == INCUBUS && (pose == MONSTER_POSE_INCUBUS_TELEPORT || pose == MONSTER_POSE_INCUBUS_TAUNT))
				|| (myStats->type == VAMPIRE && (pose == MONSTER_POSE_VAMPIRE_DRAIN || pose == MONSTER_POSE_VAMPIRE_AURA_CHARGE))
				|| (myStats->type == MIMIC 
					&& (pose == MONSTER_POSE_MIMIC_DISTURBED || pose == MONSTER_POSE_MIMIC_DISTURBED2
						|| pose == MONSTER_POSE_MIMIC_LOCKED || pose == MONSTER_POSE_MIMIC_LOCKED2
						|| pose == MONSTER_POSE_MIMIC_MAGIC1 || pose == MONSTER_POSE_MIMIC_MAGIC2))
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
				// this branch executes in special cases for certain monsters
				monsterAttack = pose;
				monsterAttackTime = 0;
				if ( multiplayer == SERVER )
				{
					// be sure to update the clients with the new wind-up pose.
					serverUpdateEntitySkill(this, 8);
					serverUpdateEntitySkill(this, 9);
				}

				if ( myStats->type == MIMIC && pose == MONSTER_POSE_MIMIC_LOCKED2 )
				{
					int lockhurt = std::max(3, (myStats->MAXHP / 20));
					if ( lockhurt > 3 )
					{
						lockhurt -= local_rng.rand() % (std::max(1, lockhurt / 4));
					}
					int oldHP = myStats->HP;
					this->modHP(-lockhurt);
					for ( int tmp = 0; tmp < 3; ++tmp )
					{
						Entity* gib = spawnGib(this);
						serverSpawnGibForClient(gib);
					}
					playSoundEntity(this, 28, 64);
					playSoundEntity(this, 152, 64);
					Entity* killer = uidToEntity(myStats->monsterMimicLockedBy);
					if ( killer )
					{
						if ( myStats->HP <= 0 && oldHP > 0 )
						{
							killer->awardXP(this, true, true);
							steamAchievementEntity(killer, "BARONY_ACH_LOCKJAW");
						}

						updateEnemyBar(killer, this, getMonsterLocalizedName(myStats->type).c_str(), myStats->HP, myStats->MAXHP, true,
							DMG_DEFAULT);
					}
				}

				return; // don't execute the attack, let the monster animation call the attack() function again.
			}
			else if ( myStats->type == BUGBEAR && pose == MONSTER_POSE_BUGBEAR_SHIELD )
			{
				monsterAttack = pose;
			}
			else if ( myStats->type == VAMPIRE && pose == MONSTER_POSE_VAMPIRE_AURA_CAST )
			{
				monsterAttack = 0;
			}
			else if ( myStats->weapon != nullptr 
				|| myStats->type == CRYSTALGOLEM || myStats->type == COCKATRICE
				|| myStats->type == MIMIC )
			{
				monsterAttack = pose;
			}
			else
			{
				monsterAttack = 1;    // punching
			}
			monsterAttackTime = 0;
		}

		// special AoE attack.
		if ( behavior == &actPlayer && pose == MONSTER_POSE_AUTOMATON_MALFUNCTION )
		{
			list_t* aoeTargets = nullptr;
			getTargetsAroundEntity(this, this, 24, PI, MONSTER_TARGET_ALL, &aoeTargets);
			if ( aoeTargets )
			{
				for ( node = aoeTargets->first; node != NULL; node = node->next )
				{
					Entity* tmpEntity = (Entity*)node->element;
					if ( tmpEntity != nullptr )
					{
						spawnExplosion(tmpEntity->x, tmpEntity->y, tmpEntity->z);
						Stat* tmpStats = tmpEntity->getStats();
						if ( tmpStats )
						{
							int explodeDmg = (10 + local_rng.rand() % 10 + myStats->LVL) * Entity::getDamageTableMultiplier(tmpEntity, *tmpStats, DAMAGE_TABLE_MAGIC); // check base magic damage resist.
							Entity* gib = spawnGib(tmpEntity);
							serverSpawnGibForClient(gib);
							if ( tmpEntity->behavior == &actPlayer )
							{
								playerhit = tmpEntity->skill[2];
								if ( playerhit > 0 && multiplayer == SERVER && !players[playerhit]->isLocalPlayer() )
								{
									strcpy((char*)net_packet->data, "SHAK");
									net_packet->data[4] = 20; // turns into .1
									net_packet->data[5] = 20;
									net_packet->address.host = net_clients[playerhit - 1].host;
									net_packet->address.port = net_clients[playerhit - 1].port;
									net_packet->len = 6;
									sendPacketSafe(net_sock, -1, net_packet, playerhit - 1);
								}
								else if ( playerhit == 0 || (splitscreen && playerhit > 0) )
								{
									cameravars[playerhit].shakex += 0.2;
									cameravars[playerhit].shakey += 20;
								}
								if ( playerhit >= 0 )
								{
									Uint32 color = makeColorRGB(255, 0, 0);
									messagePlayerColor(playerhit, MESSAGE_STATUS, color, Language::get(2523));
								}
							}
							tmpEntity->modHP(-explodeDmg);
						}
					}
				}
				//Free the list.
				list_FreeAll(aoeTargets);
				free(aoeTargets);
			}
			return;
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
				if (myStats->type != SLIME && myStats->type != RAT && myStats->type != SCARAB && myStats->type != BAT_SMALL) {
				    serverUpdateEntitySkill(this, 9);
				}
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
		/*if ( myStats->type == SHOPKEEPER )
		{
			if ( Entity* myTarget = uidToEntity(monsterTarget) )
			{
				if ( Stat* targetStats = myTarget->getStats() )
				{
					if ( targetStats->type == SHOPKEEPER )
					{
						this->monsterReleaseAttackTarget(true);
					}
				}
			}
		}*/

		bool shapeshifted = false;
		if ( this->behavior == &actPlayer && this->effectShapeshift != NOTHING )
		{
			shapeshifted = true;
		}

		bool isIllusion = false;
		if ( myStats->type == INCUBUS
			&& !strncmp(myStats->name, "inner demon", strlen("inner demon")) )
		{
			isIllusion = true;
		}

		if ( myStats->weapon != nullptr && !(myStats->type == BUGBEAR && pose == MONSTER_POSE_BUGBEAR_SHIELD)
			&& (!shapeshifted || (shapeshifted && myStats->type == CREATURE_IMP && itemCategory(myStats->weapon) == MAGICSTAFF)) )
		{
			// if non-shapeshifted, or you're an imp with a staff then process throwing/magic weapons

			// magical weapons
			if ( myStats->weapon->type == TOOL_BEARTRAP )
			{
				Item* item = myStats->weapon;
				item_ToolBeartrap(item, this);
				return;
			}
			else if ( itemCategory(myStats->weapon) == SPELLBOOK || itemCategory(myStats->weapon) == MAGICSTAFF )
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
						case MAGICSTAFF_POISON:
							castSpell(uid, &spell_poison, true, false);
							break;
						default:
							messagePlayer(player, MESSAGE_DEBUG | MESSAGE_MISC, "This is my wish stick! Wishy wishy wish!");
							break;
					}

					if ( behavior == &actPlayer )
					{
						Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_MAGICSTAFF_CASTS, myStats->weapon->type, 1);
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
							if ( myStats->weapon->status <= WORN )
							{
								forceDegrade = true;
							}
						}
					}

					if ( (local_rng.rand() % 3 == 0 && degradeWeapon && !(svFlags & SV_FLAG_HARDCORE)) || forceDegrade
						|| ((svFlags & SV_FLAG_HARDCORE) && local_rng.rand() % 6 == 0 && degradeWeapon) )
					{
						if ( player >= 0 && players[player]->isLocalPlayer() )
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
							messagePlayer(player, MESSAGE_EQUIPMENT, Language::get(659));
							if ( behavior == &actPlayer )
							{
								Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_DEGRADED, myStats->weapon->type, 1);
							}
						}
						else
						{
							if ( itemCategory(myStats->weapon) == MAGICSTAFF && myStats->weapon->beatitude < 0 )
							{
								steamAchievementClient(player, "BARONY_ACH_ONE_MANS_TRASH");
							}
							if ( behavior == &actPlayer )
							{
								Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_BROKEN, myStats->weapon->type, 1);
							}
							messagePlayer(player, MESSAGE_EQUIPMENT, Language::get(660));
							if ( player >= 0 && players[player]->isLocalPlayer() && client_classes[player] == CLASS_MESMER )
							{
								if ( myStats->weapon->type == MAGICSTAFF_CHARM )
								{
									bool foundCharmSpell = false;
									for ( node_t* spellnode = stats[player]->inventory.first; spellnode != nullptr; spellnode = spellnode->next )
									{
										Item* item = (Item*)spellnode->element;
										if ( item && itemCategory(item) == SPELL_CAT )
										{
											spell_t* spell = getSpellFromItem(player, item, false);
											if ( spell && spell->ID == SPELL_CHARM_MONSTER )
											{
												foundCharmSpell = true;
												break;
											}
										}
									}
									if ( !foundCharmSpell )
									{
										steamAchievement("BARONY_ACH_WHAT_NOW");
									}
								}
							}
						}
						if ( player > 0 && multiplayer == SERVER && !players[player]->isLocalPlayer() )
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
							if ( myStats->type == SPELLBOT )
							{
								Entity* cast = castSpell(uid, &spell_forcebolt, true, false);
								if ( cast )
								{
									cast->z -= 1;
									cast->x = this->x + 2 * cos(this->yaw);
									cast->y = this->y + 2 * sin(this->yaw);
								}
							}
							else
							{
								castSpell(uid, &spell_forcebolt, true, false);
							}
							break;
						case SPELLBOOK_MAGICMISSILE:
							if ( myStats->type == SPELLBOT )
							{
								Entity* cast = castSpell(uid, &spell_magicmissile, true, false);
								if ( cast )
								{
									cast->z -= 1;
									cast->x = this->x + 2 * cos(this->yaw);
									cast->y = this->y + 2 * sin(this->yaw);
								}
							}
							else
							{
								castSpell(uid, &spell_magicmissile, true, false);
							}
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
						{
							spell_t* spell = &spell_sleep;
							if ( Entity* target = uidToEntity(this->monsterTarget) )
							{
								if ( Stat* stats = target->getStats() )
								{
									if ( target->behavior == &actPlayer && stats->EFFECTS[EFF_ASLEEP] )
									{
										spell = &spell_magicmissile;
									}
								}
							}
							castSpell(uid, spell, true, false);
							break;
						}
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
						case SPELLBOOK_POISON:
							castSpell(uid, &spell_poison, true, false);
							break;
						case SPELLBOOK_SPRAY_WEB:
							castSpell(uid, &spell_sprayWeb, true, false);
							break;
						case SPELLBOOK_SPEED:
							castSpell(uid, &spell_speed, true, false);
							break;
						case SPELLBOOK_HEALING:
							castSpell(uid, &spell_healing, true, false);
							break;
						case SPELLBOOK_EXTRAHEALING:
							castSpell(uid, &spell_extrahealing, true, false);
							break;
						case SPELLBOOK_TROLLS_BLOOD:
							castSpell(uid, &spell_trollsBlood, true, false);
							break;
						case SPELLBOOK_REFLECT_MAGIC:
							castSpell(uid, &spell_reflectMagic, true, false);
							break;
						case SPELLBOOK_DASH:
							castSpell(uid, &spell_dash, true, false);
							break;
						case SPELLBOOK_FEAR:
							castSpell(uid, &spell_fear, true, false);
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
			else if ( isRangedWeapon(*myStats->weapon) )
			{
				if ( behavior == &actPlayer )
				{
					Compendium_t::Events_t::eventUpdate(skill[2], Compendium_t::CPDM_SHOTS_FIRED,
						stats[skill[2]]->weapon->type, 1);
				}

				// damage weapon if applicable
				int bowDegradeChance = 50;
				if ( behavior == &actPlayer )
				{
					bowDegradeChance += (stats[skill[2]]->getModifiedProficiency(PRO_RANGED) / 20) * 10;
				}
				if ( myStats->type == GOBLIN )
				{
					bowDegradeChance += 20;
					if ( myStats->getModifiedProficiency(PRO_RANGED) < SKILL_LEVEL_LEGENDARY )
					{
						bowDegradeChance = std::min(bowDegradeChance, 90);
					}
				}
				if ( myStats->type == SKELETON && behavior == &actMonster && monsterAllySummonRank > 0 )
				{
					bowDegradeChance = 100; // conjured skeleton weapon doesn't break.
				}
				if ( bowDegradeChance < 100 && local_rng.rand() % bowDegradeChance == 0 && myStats->weapon->type != ARTIFACT_BOW )
				{
					if ( myStats->weapon != NULL )
					{
						if ( player >= 0 && players[player]->isLocalPlayer() )
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
							messagePlayer(player, MESSAGE_EQUIPMENT, Language::get(661), myStats->weapon->getName());
						}
						else
						{
							playSoundEntity(this, 76, 64);
							messagePlayer(player, MESSAGE_EQUIPMENT, Language::get(662), myStats->weapon->getName());
						}
						if ( player > 0 && multiplayer == SERVER && !players[player]->isLocalPlayer() )
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
				Entity* entity = nullptr;
				if ( myStats->weapon->type == SLING )
				{
					entity = newEntity(78, 1, map.entities, nullptr); // rock
					playSoundEntity(this, 239 + local_rng.rand() % 3, 96);
				}
				else if ( myStats->weapon->type == CROSSBOW || myStats->weapon->type == HEAVY_CROSSBOW )
				{
					entity = newEntity(167, 1, map.entities, nullptr); // bolt
					if ( myStats->weapon->type == HEAVY_CROSSBOW )
					{
						playSoundEntity(this, 411 + local_rng.rand() % 3, 128);
						if ( this->behavior == &actPlayer && this->skill[2] > 0 )
						{
							this->setEffect(EFF_KNOCKBACK, true, 30, false);
						}
					}
					else
					{
						playSoundEntity(this, 239 + local_rng.rand() % 3, 96);
					}
				}
				else
				{
					entity = newEntity(166, 1, map.entities, nullptr); // arrow
					playSoundEntity(this, 239 + local_rng.rand() % 3, 96);
				}
				if ( !entity )
				{
					return;
				}
				entity->parent = uid;
				entity->x = x;
				entity->y = y;
				entity->z = z;
				if ( myStats->type == SENTRYBOT )
				{
					entity->z -= 1;
				}
				if ( myStats->type == GNOME )
				{
					entity->z -= 2;
				}
				entity->yaw = yaw;
				entity->sizex = 1;
				entity->sizey = 1;
				entity->behavior = &actArrow;
				entity->flags[UPDATENEEDED] = true;
				entity->flags[PASSABLE] = true;

				// set properties of the arrow.
				if ( pose == MONSTER_POSE_RANGED_SHOOT2 && myStats->weapon->type == ARTIFACT_BOW )
				{
					entity->setRangedProjectileAttack(*this, *myStats, QUIVER_SILVER + local_rng.rand() % 7);
				}
				else
				{
					entity->setRangedProjectileAttack(*this, *myStats);
				}

				if ( entity->arrowQuiverType != 0 && myStats->shield && itemTypeIsQuiver(myStats->shield->type) )
				{
					//TODO: Refactor this so that we don't have to copy paste this check a million times whenever some-one uses up an item.
					if ( behavior == &actPlayer && pose != MONSTER_POSE_RANGED_SHOOT2 )
					{
						if ( players[skill[2]]->isLocalPlayer() )
						{
							Compendium_t::Events_t::eventUpdate(skill[2], Compendium_t::CPDM_AMMO_FIRED,
								myStats->shield->type, 1);
						}

						myStats->shield->count--;
						if ( myStats->shield->count <= 0 )
						{
							if ( myStats->shield->node )
							{
								list_RemoveNode(myStats->shield->node);
							}
							else
							{
								free(myStats->shield);
							}
							myStats->shield = nullptr;
						}
					}
				}
				return;
			}

			// potions & gems (throwing), and thrown weapons
			if ( itemCategory(myStats->weapon) == POTION 
				|| itemCategory(myStats->weapon) == GEM 
				|| itemCategory(myStats->weapon) == THROWN
				|| myStats->weapon->type == FOOD_CREAMPIE
				|| itemIsThrowableTinkerTool(myStats->weapon) )
			{
				bool drankPotion = false;
				if ( behavior == &actMonster && myStats->type == GOATMAN && itemCategory(myStats->weapon) == POTION )
				{
					//Goatmen chug potions & then toss them at you.
					if ( myStats->weapon->type == POTION_BOOZE && !myStats->EFFECTS[EFF_DRUNK] )
					{
						item_PotionBooze(myStats->weapon, this, this, false);
						drankPotion = true;
					}
					else if ( myStats->weapon->type == POTION_HEALING )
					{
						item_PotionHealing(myStats->weapon, this, this, false);
						drankPotion = true;
					}
					else if ( myStats->weapon->type == POTION_EXTRAHEALING )
					{
						item_PotionExtraHealing(myStats->weapon, this, this, false);
						drankPotion = true;
					}
				}

				if ( myStats->weapon->type == BOOMERANG )
				{
					playSoundEntity(this, 75, 64);
					//playSoundEntity(this, 427 + local_rng.rand() % 4, 128);

				}
				else
				{
					playSoundEntity(this, 75, 64);
				}
				Entity* entity = nullptr;
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
					real_t speed = 5.f;
					real_t normalisedCharge = (charge * 1.5 / MAXCHARGE); // 0-1.5
					if ( myStats->weapon->type == BOOMERANG )
					{
						speed = 3.75 + normalisedCharge; //3.75
					}
					else
					{
						speed = 5.f + normalisedCharge;
					}

					// thrown items have slightly faster velocities
					if ( (myStats->weapon->type == STEEL_CHAKRAM || myStats->weapon->type == CRYSTAL_SHURIKEN) )
					{
						if ( this->behavior == &actPlayer )
						{
							// todo: change velocity of chakram/shuriken?
							entity->vel_x = speed * cos(players[player]->entity->yaw);
							entity->vel_y = speed * sin(players[player]->entity->yaw);
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
					else if ( myStats->weapon->type == BOOMERANG )
					{
						entity->sprite = 977;
						entity->pitch = PI;
						entity->yaw -= PI / 2;
						if ( this->behavior == &actPlayer )
						{
							entity->vel_x = speed * cos(players[player]->entity->yaw);
							entity->vel_y = speed * sin(players[player]->entity->yaw);
							entity->vel_z = -.1;
						}
						else if ( this->behavior == &actMonster )
						{
							entity->vel_x = 6 * cos(this->yaw);
							entity->vel_y = 6 * sin(this->yaw);
							entity->vel_z = -.1;
						}
					}
					else
					{
						if ( this->behavior == &actPlayer )
						{
							entity->vel_x = speed * cos(players[player]->entity->yaw);
							entity->vel_y = speed * sin(players[player]->entity->yaw);
							entity->vel_z = -.3;
						}
						else if ( this->behavior == &actMonster )
						{
							entity->vel_x = 6 * cos(this->yaw);
							entity->vel_y = 6 * sin(this->yaw);
							entity->vel_z = -.3;
						}
					}
					entity->thrownProjectilePower = this->getThrownAttack();
					if ( behavior == &actPlayer )
					{
						entity->thrownProjectileCharge = normalisedCharge * 10;
					}
				}
				else if ( itemIsThrowableTinkerTool(myStats->weapon) )
				{
					real_t normalisedCharge = (charge * 0.5);
					normalisedCharge /= MAXCHARGE;
					entity->sizex = 4;
					entity->sizey = 4;
					if ( myStats->weapon->type >= TOOL_BOMB && myStats->weapon->type <= TOOL_TELEPORT_BOMB )
					{
						entity->sizex = 2;
						entity->sizey = 2;
					}
					if ( behavior == &actPlayer )
					{
						entity->vel_x = (1.f + normalisedCharge) * cos(players[player]->entity->yaw);
						entity->vel_y = (1.f + normalisedCharge) * sin(players[player]->entity->yaw);
					}
					entity->vel_z = -.3;
					entity->roll -= (PI / 2 - 0.1 + (local_rng.rand() % 10) * 0.02);
					if ( myStats->type == GYROBOT )
					{
						entity->vel_x = 0.0;
						entity->vel_y = 0.0;
						if ( monsterAllyGetPlayerLeader() )
						{
							entity->parent = monsterAllyGetPlayerLeader()->getUID();
						}
					}
				}
				else
				{
					real_t speed = 5.f;
					if ( itemCategory(myStats->weapon) == GEM )
					{
						real_t normalisedCharge = (charge * 1.5 / MAXCHARGE); // 0-1.5
						speed = 3.f + normalisedCharge;
						if ( behavior == &actPlayer )
						{
							entity->thrownProjectileCharge = normalisedCharge * 10;
						}
					}
					entity->thrownProjectilePower = this->getThrownAttack();
					if ( this->behavior == &actPlayer )
					{
						entity->vel_x = speed * cos(players[player]->entity->yaw);
						entity->vel_y = speed * sin(players[player]->entity->yaw);
						entity->vel_z = -.5;
					}
					else if ( this->behavior == &actMonster )
					{
						entity->vel_x = speed * cos(this->yaw);
						entity->vel_y = speed * sin(this->yaw);
						entity->vel_z = -.5;
					}
				}

				if ( behavior == &actPlayer )
				{
					Compendium_t::Events_t::eventUpdate(skill[2], Compendium_t::CPDM_THROWN,
						stats[skill[2]]->weapon->type, 1);
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
		bool whip = myStats->weapon && myStats->weapon->type == TOOL_WHIP;
		bool miss = false;
		// normal attacks
		if ( target == nullptr )
		{
			if ( whip )
			{
				dist = lineTrace(this, x, y, yaw, STRIKERANGE * 1.5, LINETRACE_ATK_CHECK_FRIENDLYFIRE, false);
				playSoundEntity(this, 23 + local_rng.rand() % 5, 128); // whoosh noise
			}
			else
			{
				playSoundEntity(this, 23 + local_rng.rand() % 5, 128); // whoosh noise
				dist = lineTrace(this, x, y, yaw, STRIKERANGE, LINETRACE_ATK_CHECK_FRIENDLYFIRE, false);
			}

			if ( hit.entity && (hit.entity->behavior == &actMonster || hit.entity->behavior == &actPlayer) )
			{
				Stat* hitstats = hit.entity->getStats();
				bool bat = hitstats && hitstats->type == BAT_SMALL;
				if ( bat && hit.entity->isUntargetableBat() )
				{
					miss = true;
				}
				else if ( bat && hit.entity->monsterSpecialState == BAT_REST )
				{
					miss = false;
				}
				else if ( bat || (hitstats && hitstats->EFFECTS[EFF_AGILITY]) )
				{
					Sint32 previousMonsterState = hit.entity->monsterState;
					bool backstab = false;
					bool flanking = false;
					real_t hitAngle = hit.entity->yawDifferenceFromEntity(this);
					if ( (hitAngle >= 0 && hitAngle <= 2 * PI / 3) ) // 120 degree arc
					{
						if ( hit.entity->behavior == &actPlayer )
						{
							if ( local_rng.rand() % 2 == 0 )
							{
								flanking = true;
							}
						}
						else
						{
							if ( previousMonsterState == MONSTER_STATE_WAIT
								|| previousMonsterState == MONSTER_STATE_PATH
								|| (previousMonsterState == MONSTER_STATE_HUNT && uidToEntity(monsterTarget) == nullptr) )
							{
								// unaware monster, get backstab damage.
								backstab = true;
							}
							else if ( local_rng.rand() % 2 == 0 )
							{
								// monster currently engaged in some form of combat maneuver
								// 1 in 2 chance to flank defenses.
								flanking = true;
							}
						}
					}

					if ( backstab )
					{
						miss = false;
					}
					else
					{
						int baseChance = bat ? 6 : 3;
						if ( flanking )
						{
							baseChance = std::max(1, baseChance - 2);
						}
						miss = local_rng.rand() % 10 < baseChance;
					}

					if ( myStats->weapon )
					{
						if ( myStats->weapon->type == ARTIFACT_SPEAR && !shapeshifted )
						{
							miss = false;
						}
					}
				}

				if ( miss )
				{
					if ( !hit.entity->isUntargetableBat() )
					{
						if ( player >= 0 || (behavior == &actMonster && monsterAllyGetPlayerLeader())
							|| hit.entity->behavior == &actPlayer || hit.entity->monsterAllyGetPlayerLeader() )
						{
							spawnDamageGib(hit.entity, 0, DamageGib::DMG_MISS, DamageGibDisplayType::DMG_GIB_MISS, true);
						}
						if ( player >= 0 && bat )
						{
							steamStatisticUpdateClient(player, STEAM_STAT_PITCH_PERFECT, STEAM_STAT_INT, 1);
						}

						if ( hit.entity->behavior == &actPlayer )
						{
							messagePlayerColor(hit.entity->skill[2], MESSAGE_COMBAT, makeColorRGB(0, 255, 0), Language::get(6286));
						}
						else if ( hit.entity->behavior == &actMonster )
						{
							bool doHitAlert = true;
							if ( !(svFlags & SV_FLAG_FRIENDLYFIRE) )
							{
								// test for friendly fire
								if ( checkFriend(hit.entity) )
								{
									doHitAlert = false;
								}
							}

							if ( doHitAlert )
							{
								Stat* hitstats = hit.entity->getStats();
								if ( hitstats )
								{
									bool alertTarget = true;
									if ( behavior == &actMonster && monsterAllyIndex != -1 && hit.entity->monsterAllyIndex != -1 )
									{
										// if we're both allies of players, don't alert the hit target.
										alertTarget = false;
									}

									// alert the monster!
									if ( hit.entity->monsterState != MONSTER_STATE_ATTACK )
									{
										if ( alertTarget && hit.entity->monsterSpecialState == 0 )
										{
											hit.entity->monsterAcquireAttackTarget(*this, MONSTER_STATE_PATH, true);
										}
									}

									// alert other monsters too
									if ( alertTarget )
									{
										hit.entity->alertAlliesOnBeingHit(this);
									}
									hit.entity->updateEntityOnHit(this, alertTarget);
								}
							}
						}
					}

					hit.entity = nullptr;
				}
			}
		}
		else
		{
			hit.entity = target;
		}

		if ( hit.entity != nullptr )
		{
			bool mimic = hit.entity->isInertMimic();

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

			Sint32 previousMonsterState = -1;
			Sint32 previousMonsterSpecialState = -1;

			if ( hit.entity->behavior == &actBoulder )
			{
				if ( (myStats->weapon != nullptr && !shapeshifted) || pose == PLAYER_POSE_GOLEM_SMASH )
				{
					if ( (myStats->weapon && myStats->weapon->type == TOOL_PICKAXE && !shapeshifted) || pose == PLAYER_POSE_GOLEM_SMASH )
					{
						// spawn several rock items
						if ( pose == PLAYER_POSE_GOLEM_SMASH )
						{
							createParticleRock(hit.entity);
							if ( multiplayer == SERVER )
							{
								serverSpawnMiscParticles(hit.entity, PARTICLE_EFFECT_ABILITY_ROCK, 78);
							}
						}
						else
						{
							int i = 8 + local_rng.rand() % 4;
							int c;
							for ( c = 0; c < i; c++ )
							{
								Entity* entity = newEntity(-1, 1, map.entities, nullptr); //Rock/item entity.
								entity->flags[INVISIBLE] = true;
								entity->flags[UPDATENEEDED] = true;
								entity->x = hit.entity->x - 4 + local_rng.rand() % 8;
								entity->y = hit.entity->y - 4 + local_rng.rand() % 8;
								entity->z = -6 + local_rng.rand() % 12;
								entity->sizex = 4;
								entity->sizey = 4;
								entity->yaw = local_rng.rand() % 360 * PI / 180;
								entity->vel_x = (local_rng.rand() % 20 - 10) / 10.0;
								entity->vel_y = (local_rng.rand() % 20 - 10) / 10.0;
								entity->vel_z = -.25 - (local_rng.rand() % 5) / 10.0;
								entity->flags[PASSABLE] = true;
								entity->behavior = &actItem;
								entity->flags[USERFLAG1] = true; // no collision: helps performance
								entity->skill[10] = GEM_ROCK;    // type
								entity->skill[11] = WORN;        // status
								entity->skill[12] = 0;           // beatitude
								entity->skill[13] = 1;           // count
								entity->skill[14] = 0;           // appearance
								entity->skill[15] = 1;			 // identified
							}
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

						boulderLavaOrArcaneOnDestroy(hit.entity, hit.entity->sprite, nullptr);

						// destroy the boulder
						playSoundEntity(hit.entity, 67, 128);
						list_RemoveNode(hit.entity->mynode);
						messagePlayer(player, MESSAGE_COMBAT, Language::get(663));

						if ( myStats->weapon && myStats->weapon->type == TOOL_PICKAXE && !shapeshifted && pose != PLAYER_POSE_GOLEM_SMASH )
						{
							Compendium_t::Events_t::eventUpdate(skill[2], Compendium_t::CPDM_PICKAXE_BOULDERS_DUG, myStats->weapon->type, 1);
						}

						if ( myStats->weapon && local_rng.rand() % 2 && pose != PLAYER_POSE_GOLEM_SMASH )
						{
							myStats->weapon->status = static_cast<Status>(myStats->weapon->status - 1);
							if ( myStats->weapon->status < BROKEN )
							{
								myStats->weapon->status = BROKEN; // bounds checking.
							}
							if ( myStats->weapon->status == BROKEN )
							{
								messagePlayer(player, MESSAGE_COMBAT, Language::get(664));
								playSoundEntity(this, 76, 64);

								if ( behavior == &actPlayer )
								{
									if ( myStats->weapon && myStats->weapon->type == TOOL_PICKAXE )
									{
										Compendium_t::Events_t::eventUpdate(skill[2], Compendium_t::CPDM_BROKEN, myStats->weapon->type, 1);
									}
								}
							}
							else
							{
								messagePlayer(player, MESSAGE_COMBAT, Language::get(665));
								if ( behavior == &actPlayer && myStats->weapon )
								{
									Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_DEGRADED, myStats->weapon->type, 1);
								}
							}
							if ( player > 0 && multiplayer == SERVER && !players[player]->isLocalPlayer() )
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
							if ( local_rng.rand() % 2 == 0 )
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
									Uint32 color = makeColorRGB(255, 128, 0);
									messagePlayerColor(c, MESSAGE_HINT, color, Language::get(406));
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
			else if ( hit.entity->behavior == &actMonster && !mimic )
			{
				previousMonsterState = hit.entity->monsterState;
				previousMonsterSpecialState = hit.entity->monsterSpecialState;
				hitstats = hit.entity->getStats();
				if ( hitstats )
				{
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
					if ( alertTarget )
					{
						hit.entity->alertAlliesOnBeingHit(this);
					}
					hit.entity->updateEntityOnHit(this, alertTarget);
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
					Entity* entity = nullptr;
					if ( c )
					{
						entity = uidToEntity(*c);
					}
					Entity* ohitentity = hit.entity;
					if ( entity )
					{
						Stat* buddystats = entity->getStats();
						if ( buddystats != nullptr )
						{
							if ( entity->monsterState == MONSTER_STATE_WAIT || (entity->monsterState == MONSTER_STATE_HUNT && entity->monsterTarget != uid) ) // monster is waiting or hunting
							{
								if ( entity->monsterAllyState == ALLY_STATE_DEFEND )
								{
									// monster is defending, make em stay put unless line of sight.
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
								else
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
					hit.entity = ohitentity;
				}
			}
			else if ( hit.entity->behavior == &actColliderDecoration
				&& hit.entity->colliderDiggable != 0
				&& ((myStats->weapon && myStats->weapon->type == TOOL_PICKAXE && !shapeshifted) || pose == PLAYER_POSE_GOLEM_SMASH) )
			{
				magicDig(this, nullptr, 1, 0);

				if ( myStats->weapon && local_rng.rand() % 2 && pose != PLAYER_POSE_GOLEM_SMASH )
				{
					myStats->weapon->status = static_cast<Status>(myStats->weapon->status - 1);
					if ( myStats->weapon->status < BROKEN )
					{
						myStats->weapon->status = BROKEN; // bounds checking.
					}
					if ( myStats->weapon->status == BROKEN )
					{
						messagePlayer(player, MESSAGE_COMBAT, Language::get(664));
						playSoundEntity(this, 76, 64);

						if ( behavior == &actPlayer )
						{
							if ( myStats->weapon && myStats->weapon->type == TOOL_PICKAXE )
							{
								Compendium_t::Events_t::eventUpdate(skill[2], Compendium_t::CPDM_BROKEN, myStats->weapon->type, 1);
							}
						}
					}
					else
					{
						messagePlayer(player, MESSAGE_COMBAT, Language::get(665));
						if ( behavior == &actPlayer && myStats->weapon )
						{
							Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_DEGRADED, myStats->weapon->type, 1);
						}
					}
					if ( player > 0 && multiplayer == SERVER && !players[player]->isLocalPlayer() )
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
			else if ( hit.entity->behavior == &actDoor || hit.entity->behavior == &::actFurniture || hit.entity->behavior == &::actChest
				|| mimic
				|| (hit.entity->isDamageableCollider() && hit.entity->isColliderDamageableByMelee()) )
			{
				if ( mimic )
				{
					hitstats = hit.entity->getStats();
				}

				int axe = 0;
				int damage = 1;
				int weaponskill = -1;
				if ( myStats->weapon && !shapeshifted )
				{
					weaponskill = getWeaponSkill(myStats->weapon);
					if ( hit.entity->behavior == &actColliderDecoration )
					{
						if ( weaponskill >= 0 && hit.entity->isColliderResistToSkill(weaponskill) )
						{
							damage = 1;
						}
						else
						{
							damage = 2 + local_rng.rand() % 3;
						}
						if ( weaponskill >= 0 && hit.entity->isColliderWeakToSkill(weaponskill) )
						{
							axe = 2 * (myStats->getModifiedProficiency(weaponskill) / 20);
							axe = std::min(axe, 9);
						}
					}
					else if ( weaponskill == PRO_AXE )
					{
						axe = (myStats->getModifiedProficiency(weaponskill) / 20);
						if ( myStats->getModifiedProficiency(weaponskill) >= SKILL_LEVEL_LEGENDARY )
						{
							axe = 9;
						}
						axe = std::min(axe, 9);
					}
				}
				else
				{
					weaponskill = PRO_UNARMED;
					if ( hit.entity->behavior == &actColliderDecoration )
					{
						if ( hit.entity->isColliderResistToSkill(weaponskill) )
						{
							damage = 1;
						}
						else
						{
							damage = 2 + local_rng.rand() % 3;
						}
						if ( hit.entity->isColliderWeakToSkill(weaponskill) )
						{
							axe = 2 * (myStats->getModifiedProficiency(weaponskill) / 20);
							axe = std::min(axe, 9);
						}
					}
					else if ( hit.entity->behavior != &::actChest && !mimic )
					{
						axe = (myStats->getModifiedProficiency(weaponskill) / 20);
						if ( myStats->getModifiedProficiency(weaponskill) >= SKILL_LEVEL_LEGENDARY )
						{
							axe = 9;
						}
						axe = std::min(axe, 9);
					}
				}
				if ( pose == PLAYER_POSE_GOLEM_SMASH )
				{
					if ( hit.entity->behavior == &actDoor || hit.entity->behavior == &::actFurniture )
					{
						axe += 20;
					}
					else if ( hit.entity->behavior == &::actChest || mimic )
					{
						axe = std::min(axe + 50, 50);
					}
					damage += axe;
				}
				else
				{
					damage += axe;
					if ( charge >= MAXCHARGE / 2 )
					{
						damage *= 2;
					}
				}

				int& entityHP = hit.entity->behavior == &actColliderDecoration ? hit.entity->colliderCurrentHP :
					(hit.entity->behavior == &::actChest ? hit.entity->chestHealth :
					(hit.entity->behavior == &actDoor ? hit.entity->doorHealth : 
					((mimic && hitstats) ? hitstats->HP :
						hit.entity->furnitureHealth)));
				int oldHP = entityHP;
				entityHP -= damage;

				if ( whip )
				{
					playSoundEntity(hit.entity, 407 + local_rng.rand() % 3, 64);
				}
				else
				{
					int sound = 28; //damage.ogg
					if ( hit.entity->behavior == &actColliderDecoration && hit.entity->getColliderSfxOnHit() > 0 )
					{
						sound = hit.entity->getColliderSfxOnHit();
					}
					playSoundEntity(hit.entity, sound, 64);
				}
				if ( entityHP > 0 )
				{
					if ( hit.entity->behavior == &actDoor )
					{
						messagePlayer(player, MESSAGE_COMBAT_BASIC, Language::get(666));
					}
					else if ( hit.entity->behavior == &::actChest )
					{
						messagePlayer(player, MESSAGE_COMBAT_BASIC, Language::get(667));
					}
					else if ( mimic )
					{
						messagePlayer(player, MESSAGE_COMBAT_BASIC, Language::get(667));
						previousMonsterState = hit.entity->monsterState;
						bool alertTarget = true;
						if ( behavior == &actMonster && monsterAllyIndex != -1 && hit.entity->monsterAllyIndex != -1 )
						{
							// if we're both allies of players, don't alert the hit target.
							alertTarget = false;
						}

						// alert the monster!
						if ( hit.entity->monsterState != MONSTER_STATE_ATTACK )
						{
							if ( alertTarget )
							{
								hit.entity->monsterAcquireAttackTarget(*this, MONSTER_STATE_PATH, true);
							}
						}

						hit.entity->updateEntityOnHit(this, alertTarget);
					}
					else if ( hit.entity->isDamageableCollider() )
					{
						messagePlayer(player, MESSAGE_COMBAT_BASIC, Language::get(hit.entity->getColliderOnHitLangEntry()),
							Language::get(hit.entity->getColliderLangName()));
					}
					else if ( hit.entity->behavior == &::actFurniture )
					{
						switch ( hit.entity->furnitureType )
						{
							case FURNITURE_CHAIR:
								messagePlayer(player, MESSAGE_COMBAT_BASIC, Language::get(669));
								break;
							case FURNITURE_TABLE:
								messagePlayer(player, MESSAGE_COMBAT_BASIC, Language::get(668));
								break;
							case FURNITURE_BED:
								messagePlayer(player, MESSAGE_COMBAT_BASIC, Language::get(2509), Language::get(2505));
								break;
							case FURNITURE_BUNKBED:
								messagePlayer(player, MESSAGE_COMBAT_BASIC, Language::get(2509), Language::get(2506));
								break;
							case FURNITURE_PODIUM:
								messagePlayer(player, MESSAGE_COMBAT_BASIC, Language::get(2509), Language::get(2507));
								break;
							default:
								break;
						}
					}
				}
				else
				{
					entityHP = 0;
					if ( hit.entity->behavior == &actDoor )
					{
						messagePlayer(player, MESSAGE_COMBAT, Language::get(670));
						if ( !hit.entity->skill[0] )
						{
							hit.entity->skill[6] = (x > hit.entity->x);
						}
						else
						{
							hit.entity->skill[6] = (y < hit.entity->y);
						}
						Compendium_t::Events_t::eventUpdateWorld(player, Compendium_t::CPDM_DOOR_BROKEN, "door", 1);
					}
					else if ( hit.entity->behavior == &::actChest )
					{
						messagePlayer(player, MESSAGE_COMBAT, Language::get(671));
						Compendium_t::Events_t::eventUpdateWorld(player, Compendium_t::CPDM_CHESTS_DESTROYED, "chest", 1);
					}
					else if ( mimic )
					{
						messagePlayer(player, MESSAGE_COMBAT, Language::get(671));
					}
					else if ( hit.entity->isDamageableCollider() )
					{
						hit.entity->colliderKillerUid = getUID();
						messagePlayer(player, MESSAGE_COMBAT, Language::get(hit.entity->getColliderOnBreakLangEntry()),
							Language::get(hit.entity->getColliderLangName()));
						if ( hit.entity->isColliderWall() )
						{
							Compendium_t::Events_t::eventUpdateWorld(player, Compendium_t::CPDM_BARRIER_DESTROYED, "breakable barriers", 1);
						}
						if ( behavior == &actPlayer )
						{
							if ( hit.entity->isColliderBreakableContainer() )
							{
								steamStatisticUpdateClient(player, STEAM_STAT_SMASH_MELEE, STEAM_STAT_INT, 1);
							}
						}
					}
					else if ( hit.entity->behavior == &::actFurniture )
					{
						if ( oldHP > 0 && behavior == &actPlayer )
						{
							gameModeManager.currentSession.challengeRun.updateKillEvent(hit.entity);
						}
						switch ( hit.entity->furnitureType )
						{
							case FURNITURE_CHAIR:
								messagePlayer(player, MESSAGE_COMBAT, Language::get(673));
								break;
							case FURNITURE_TABLE:
								messagePlayer(player, MESSAGE_COMBAT, Language::get(672));
								break;
							case FURNITURE_BED:
								messagePlayer(player, MESSAGE_COMBAT, Language::get(2510), Language::get(2505));
								break;
							case FURNITURE_BUNKBED:
								messagePlayer(player, MESSAGE_COMBAT, Language::get(2510), Language::get(2506));
								break;
							case FURNITURE_PODIUM:
								messagePlayer(player, MESSAGE_COMBAT, Language::get(2510), Language::get(2507));
								break;
							default:
								break;
						}
					}
				}
				if ( hit.entity->behavior == &actDoor )
				{
					updateEnemyBar(this, hit.entity, Language::get(674), entityHP, hit.entity->skill[9], false,
						DamageGib::DMG_DEFAULT);
				}
				else if ( hit.entity->behavior == &::actChest )
				{
					updateEnemyBar(this, hit.entity, Language::get(675), entityHP, hit.entity->skill[8], false,
						DamageGib::DMG_DEFAULT);
				}
				else if ( hit.entity->isDamageableCollider() )
				{
					updateEnemyBar(this, hit.entity, Language::get(hit.entity->getColliderLangName()), entityHP, hit.entity->colliderMaxHP, false,
						DamageGib::DMG_DEFAULT);
				}
				else if ( hit.entity->behavior == &::actFurniture )
				{
					switch ( hit.entity->furnitureType )
					{
						case FURNITURE_CHAIR:
							updateEnemyBar(this, hit.entity, Language::get(677), entityHP, hit.entity->furnitureMaxHealth, false,
								DamageGib::DMG_DEFAULT);
							break;
						case FURNITURE_TABLE:
							updateEnemyBar(this, hit.entity, Language::get(676), entityHP, hit.entity->furnitureMaxHealth, false,
								DamageGib::DMG_DEFAULT);
							break;
						case FURNITURE_BED:
							updateEnemyBar(this, hit.entity, Language::get(2505), entityHP, hit.entity->furnitureMaxHealth, false,
								DamageGib::DMG_DEFAULT);
							break;
						case FURNITURE_BUNKBED:
							updateEnemyBar(this, hit.entity, Language::get(2506), entityHP, hit.entity->furnitureMaxHealth, false,
								DamageGib::DMG_DEFAULT);
							break;
						case FURNITURE_PODIUM:
							updateEnemyBar(this, hit.entity, Language::get(2507), entityHP, hit.entity->furnitureMaxHealth, false,
								DamageGib::DMG_DEFAULT);
							break;
						default:
							break;
					}
				}
				else if ( mimic && hitstats )
				{
					updateEnemyBar(this, hit.entity, Language::get(675), entityHP, hit.entity->getStats()->MAXHP, false,
						DamageGib::DMG_DEFAULT);

					// kill monster
					if ( hitstats->HP == 0 && oldHP > 0 )
					{
						messagePlayerMonsterEvent(player, makeColorRGB(0, 255, 0), *hitstats, Language::get(692), Language::get(692), MSG_COMBAT);
						awardXP(hit.entity, true, true);
					}
				}
			}
			else if ( hit.entity->behavior == &actSink )
			{
				if ( whip )
				{
					playSoundEntity(hit.entity, 407 + local_rng.rand() % 3, 64);
				}
				else
				{
					playSoundEntity(hit.entity, 28, 64);
				}
				playSoundEntity(hit.entity, 140 + local_rng.rand() % 2, 64);
				messagePlayer(player, MESSAGE_COMBAT, Language::get(678));
				if ( hit.entity->skill[0] > 0 )
				{
					hit.entity->skill[0]--; //Deplete one usage.

					//50% chance spawn a slime.
					if ( local_rng.rand() % 2 == 0 )
					{
						// spawn slime
						int ox = hit.entity->x / 16;
						int oy = hit.entity->y / 16;
						Entity* monster = summonMonster(SLIME, ox * 16 + 8, oy * 16 + 8);
						if ( monster )
						{
							auto& rng = hit.entity->entity_rng ? *hit.entity->entity_rng : local_rng;
							monster->seedEntityRNG(rng.getU32());
							slimeSetType(monster, monster->getStats(), true, &rng);
							messagePlayer(player, MESSAGE_HINT, Language::get(582));
							if ( behavior == &actPlayer )
							{
								Compendium_t::Events_t::eventUpdateWorld(skill[2], Compendium_t::CPDM_SINKS_SLIMES, "sink", 1);
							}
						}
					}

					if ( hit.entity->skill[0] == 0 )   //Depleted.
					{
						messagePlayer(player, MESSAGE_COMBAT, Language::get(585)); //TODO: Alert all players that see (or otherwise in range) it?
						playSoundEntity(hit.entity, 132, 64);
						serverUpdateEntitySkill(hit.entity, 0);
					}
				}
			}
			else
			{
				if ( myStats->weapon && !shapeshifted && pose != PLAYER_POSE_GOLEM_SMASH )
				{
					// bang
					spawnBang(hit.x - cos(yaw) * 2, hit.y - sin(yaw) * 2, 0);
				}
				else
				{
					playSoundPos(hit.x, hit.y, 183, 64);
				}
			}

			if ( hitstats != nullptr && !mimic )
			{
				// hit chance
				//int hitskill=5; // for unarmed combat

				if ( hit.entity->doSilkenBowOnAttack(this) )
				{
					return;
				}

				if ( behavior == &actPlayer )
				{
					if ( !players[skill[2]]->isLocalPlayer() )
					{
						if ( achievementRangedMode[skill[2]] && !playerFailedRangedOnlyConduct[skill[2]] )
						{
							messagePlayer(skill[2], MESSAGE_HINT, Language::get(3923)); // prevent attack.
							return;
						}
						if ( achievementRangedMode[skill[2]] )
						{
							messagePlayer(skill[2], MESSAGE_HINT, Language::get(3924)); // notify no longer eligible for achievement but still atk.
						}
						if ( !playerFailedRangedOnlyConduct[skill[2]] )
						{
							playerFailedRangedOnlyConduct[skill[2]] = true;
							serverUpdatePlayerConduct(skill[2], CONDUCT_RANGED_ONLY, 0);
						}
					}
					else
					{
						if ( achievementRangedMode[skill[2]] && conductGameChallenges[CONDUCT_RANGED_ONLY] )
						{
							messagePlayer(skill[2], MESSAGE_HINT, Language::get(3923)); // prevent attack.
							return;
						}
						if ( achievementRangedMode[skill[2]] )
						{
							messagePlayer(skill[2], MESSAGE_HINT, Language::get(3924)); // notify no longer eligible for achievement but still atk.
						}
						conductGameChallenges[CONDUCT_RANGED_ONLY] = 0;
					}
				}

				weaponskill = getWeaponSkill(myStats->weapon);
				//if ( behavior == &actMonster && weaponskill == PRO_UNARMED ) 
				//{
				//	was -1 for legacy monster punching and damage variance. != &actMonster added to PRO_UNARMED check for damage variability.
				//	weaponskill = -1;
				//}
				if ( shapeshifted || pose == PLAYER_POSE_GOLEM_SMASH )
				{
					weaponskill = PRO_UNARMED;
				}
				if ( pose == MONSTER_POSE_BUGBEAR_SHIELD )
				{
					weaponskill = PRO_UNARMED;
				}

				real_t weaponMultipliers = 0.0;
				if ( weaponskill == PRO_UNARMED )
				{
					weaponMultipliers = Entity::getDamageTableMultiplier(hit.entity, *hitstats, DAMAGE_TABLE_UNARMED);
				}
				else if ( weaponskill == PRO_RANGED )
				{
					weaponMultipliers = Entity::getDamageTableMultiplier(hit.entity, *hitstats, DAMAGE_TABLE_RANGED);
				}
				else if ( weaponskill >= 0 )
				{
					DamageTableType dmgType = static_cast<DamageTableType>(weaponskill - PRO_SWORD);
					weaponMultipliers = Entity::getDamageTableMultiplier(hit.entity, *hitstats, dmgType);
				}

				int thornsEffect = 0;
				if ( hitstats->mask && hitstats->mask->type == MASK_MOUTHKNIFE )
				{
					if ( hitstats->mask->beatitude >= 0 || shouldInvertEquipmentBeatitude(hitstats) )
					{
						thornsEffect = (1 + abs(hitstats->mask->beatitude)) * 2;
					}
					else
					{
						thornsEffect = -2 * (1 + abs(hitstats->mask->beatitude));
					}
				}

				bool dyrnwynSmite = false;
				bool gugnirProc = false;

				if ( weaponskill == PRO_SWORD && myStats->weapon && myStats->weapon->type == ARTIFACT_SWORD && !shapeshifted )
				{
					switch ( hitstats->type )
					{
						case SKELETON:
						case CREATURE_IMP:
						case GHOUL:
						case DEMON:
						case SUCCUBUS:
						case INCUBUS:
						case VAMPIRE:
						case LICH:
						case LICH_ICE:
						case LICH_FIRE:
						case DEVIL:
						{
							// smite these creatures
							real_t amount = 0.0;
							real_t percent = getArtifactWeaponEffectChance(myStats->weapon->type, *myStats, &amount);
							if ( local_rng.rand() % 100 < static_cast<int>(percent) )
							{
								weaponMultipliers += amount;
								dyrnwynSmite = true;
								spawnMagicEffectParticles(hit.entity->x, hit.entity->y, hit.entity->z, 981);
								//playSoundEntity(hit.entity, 249, 64);
							}
							break;
						}
						default:
							break;
					}
				}
				/*if( weaponskill>=0 )
				hitskill = myStats->PROFICIENCIES[weaponskill]/5;
				c = local_rng.rand()%20 + hitskill + (weaponskill==PRO_POLEARM);
				bool hitsuccess=false;
				if( myStats->weapon ) {
				if( myStats->weapon->type == ARTIFACT_SPEAR ) {
				hitsuccess=true; // Gungnir always lands a hit!
				}
				}
				if( c > 10+std::min(std::max(-3,hit.entity->getDEX()-my->getDEX()),3) ) {
				hitsuccess=true;
				}
				if ( hitsuccess ) */
				{
					// calculate and perform damage to opponent
					int damage = 0;
					int damagePreMultiplier = 1;

					if ( (myStats->type == CRYSTALGOLEM && pose == MONSTER_POSE_GOLEM_SMASH)
						|| (myStats->type == LICH_FIRE && pose == 3) )
					{
						damagePreMultiplier = 2;
					}
					else if ( player >= 0 && pose == PLAYER_POSE_GOLEM_SMASH )
					{
						damagePreMultiplier = 2;
					}

					int myAttack = std::max(0, (Entity::getAttack(this, myStats, behavior == &actPlayer) * damagePreMultiplier) + getBonusAttackOnTarget(*hitstats));
					if ( myStats->type == BUGBEAR && pose == MONSTER_POSE_BUGBEAR_SHIELD )
					{
						myAttack += 2 + local_rng.rand() % 3;
					}
					int enemyAC = AC(hitstats);
					if ( weaponskill == PRO_POLEARM && myStats->weapon && myStats->weapon->type == ARTIFACT_SPEAR && !shapeshifted )
					{
						real_t amount = 0.f;
						real_t percent = getArtifactWeaponEffectChance(ARTIFACT_SPEAR, *myStats, &amount);
						if ( (local_rng.rand() % 100 < static_cast<int>(percent)) )
						{
							enemyAC *= amount;
							gugnirProc = true;
						}
					}
					int numBlessings = 0;
					real_t targetACEffectiveness = Entity::getACEffectiveness(hit.entity, hitstats, hit.entity->behavior == &actPlayer, this, myStats, numBlessings);
					int attackAfterReductions = static_cast<int>(std::max(0.0, ((myAttack * targetACEffectiveness - enemyAC))) + (1.0 - targetACEffectiveness) * myAttack);
					if ( weaponskill == PRO_UNARMED )
					{
						damage = attackAfterReductions * weaponMultipliers;
					}
					else if ( weaponskill == PRO_RANGED )
					{
						damage = attackAfterReductions * weaponMultipliers;
					}
					else if ( weaponskill >= 0 )
					{
						damage = attackAfterReductions * weaponMultipliers;
					}
					else
					{
						damage = attackAfterReductions;
					}

					damage = std::max(0, damage);

					if ( weaponskill == PRO_AXE )
					{
						damage++;
						if ( myStats->type == BUGBEAR )
						{
							damage += 1;
						}
					}
					if ( myStats->type == LICH_FIRE && !hitstats->defending )
					{
						if ( damage <= 8 )
						{
							damage += (8 - damage) + local_rng.rand() % 9; // 8 - 16 minimum damage.
						}
					}
					if ( behavior == &actMonster && myStats->EFFECTS[EFF_VAMPIRICAURA] )
					{
						damage += 5; // 5 bonus damage after reductions.
					}

					bool backstab = false;
					bool flanking = false;
					if ( player >= 0 && !monsterIsImmobileTurret(hit.entity, hitstats) && !(hitstats->type == MIMIC) )
					{
						real_t hitAngle = hit.entity->yawDifferenceFromEntity(this);
						if ( (hitAngle >= 0 && hitAngle <= 2 * PI / 3) ) // 120 degree arc
						{
							int stealthCapstoneBonus = 1; 
							if ( skillCapstoneUnlockedEntity(PRO_STEALTH) )
							{
								stealthCapstoneBonus = 2;
							}

							if ( previousMonsterState == MONSTER_STATE_WAIT
								|| previousMonsterState == MONSTER_STATE_PATH
								|| (previousMonsterState == MONSTER_STATE_HUNT && uidToEntity(monsterTarget) == nullptr) )
							{
								// unaware monster, get backstab damage.
								backstab = true;
								int bonus = (stats[player]->getModifiedProficiency(PRO_STEALTH) / 20 + 2) * (2 * stealthCapstoneBonus);
								if ( myStats->helmet && myStats->helmet->type == HAT_HOOD_ASSASSIN )
								{
									if ( myStats->helmet->beatitude >= 0 || shouldInvertEquipmentBeatitude(myStats) )
									{
										bonus += std::min(4 + (2 * abs(myStats->helmet->beatitude)), 8);
									}
									else
									{
										bonus /= 2;
									}
								}
								damage += bonus;
								if ( hit.entity->behavior != &actPlayer )
								{
									if ( hitstats->type == BAT_SMALL && previousMonsterSpecialState == BAT_REST )
									{
										if ( local_rng.rand() % 10 == 0 )
										{
											this->increaseSkill(PRO_STEALTH);
										}
									}
									else
									{
										if ( local_rng.rand() % 4 > 0 )
										{
											this->increaseSkill(PRO_STEALTH);
										}
									}
								}
							}
							else if ( local_rng.rand() % 2 == 0 )
							{
								// monster currently engaged in some form of combat maneuver
								// 1 in 2 chance to flank defenses.
								flanking = true;
								damage += (stats[player]->getModifiedProficiency(PRO_STEALTH) / 20 + 1) * (stealthCapstoneBonus);
								if ( local_rng.rand() % 20 == 0 && hit.entity->behavior != &actPlayer )
								{
									this->increaseSkill(PRO_STEALTH);
								}
							}
						}
					}

					static ConsoleVariable<bool> cvar_atkonhit("/enemy_debugatkonhit", false);
					if ( (weaponskill >= PRO_SWORD && weaponskill < PRO_SHIELD) || (weaponskill == PRO_UNARMED && behavior != &actMonster) || weaponskill == PRO_RANGED )
					{
						if ( *cvar_atkonhit )
						{
							int chance = 0;
							if ( weaponskill == PRO_POLEARM )
							{
								chance = (damage / 3) * (100 - myStats->getModifiedProficiency(weaponskill)) / 100.f;
							}
							else
							{
								chance = (damage / 2) * (100 - myStats->getModifiedProficiency(weaponskill)) / 100.f;
							}
							messagePlayer(0, MESSAGE_DEBUG, "Old range minmax: %d-%d", damage - chance, damage);
						}
						/*if ( chance > 0 )
						{
							damage = (damage - chance) + (local_rng.rand() % chance) + 1;
						}*/
						real_t variance = 20;
						real_t baseSkillModifier = 50.0; // 40-60 base
						Entity::setMeleeDamageSkillModifiers(this, myStats, weaponskill, baseSkillModifier, variance, nullptr);
						real_t skillModifier = baseSkillModifier - (variance / 2) + (myStats->getModifiedProficiency(weaponskill) / 2.0);
						skillModifier += (local_rng.rand() % (1 + static_cast<int>(variance)));
						skillModifier /= 100.0;
						skillModifier = std::min(skillModifier, 1.0);
						damage = damage - static_cast<int>((1.0 - skillModifier) * damage);
						if ( *cvar_atkonhit )
						{
							messagePlayer(0, MESSAGE_DEBUG, "New Dmg: %.f%%: %d", 100.0 * skillModifier, damage);
						}
					}

					if ( *cvar_atkonhit )
					{
						if ( weaponskill >= 0 )
						{
							messagePlayer(0, MESSAGE_DEBUG, "Old damage max: %d", (int)((std::max(0, (myAttack - enemyAC)) * weaponMultipliers)));
						}
					}


					int olddamage = damage;
					const int chargeMult = std::max(charge, MAXCHARGE / 2) / ((double)(MAXCHARGE / 2));
					damage *= chargeMult;
					bool parashuProc = false;
					if ( myStats->weapon && !shapeshifted )
					{
						if ( myStats->weapon->type == ARTIFACT_AXE )
						{
							real_t amount = 0.0;
							real_t percent = getArtifactWeaponEffectChance(myStats->weapon->type, *myStats, &amount);

							if ( local_rng.rand() % 100 < static_cast<int>(percent) )
							{
								if ( !shapeshifted )
								{
									damage *= amount; // Parashu sometimes multiplier damage
								}
								parashuProc = true;
							}
						}
					}
					
					if ( hitstats->type == DUMMYBOT )
					{
						// higher level dummy bots have damage cap limits on hit.
						if ( myStats->type != MINOTAUR && myStats->type != LICH_FIRE )
						{
							if ( hitstats->LVL >= 10 )
							{
								damage = std::min(damage, 15);
							}
							else if ( hitstats->LVL >= 5 )
							{
								damage = std::min(damage, 30);
							}
						}
					}

					if ( playerhit >= 0 )
					{
						if ( hitstats->defending && damage == 0 && hitstats->shield )
						{
							Compendium_t::Events_t::eventUpdate(playerhit, Compendium_t::CPDM_BLOCKED_ATTACKS, hitstats->shield->type, 1);
							Compendium_t::Events_t::eventUpdate(playerhit, Compendium_t::CPDM_BLOCKED_HIGHEST_DMG, hitstats->shield->type, myAttack);
						}

						if ( hitstats->shield && hitstats->defending )
						{
							Compendium_t::Events_t::eventUpdateCodex(playerhit, Compendium_t::CPDM_CLASS_BLOCK_DEFENDED, "blocking", 1);
							Compendium_t::Events_t::eventUpdateCodex(playerhit, Compendium_t::CPDM_CLASS_BLOCK_DEFENDED_RUN, "blocking", 1);
						}
						else
						{
							Compendium_t::Events_t::eventUpdateCodex(playerhit, Compendium_t::CPDM_CLASS_BLOCK_UNDEFENDED, "blocking", 1);
							Compendium_t::Events_t::eventUpdateCodex(playerhit, Compendium_t::CPDM_CLASS_BLOCK_UNDEFENDED_RUN, "blocking", 1);
						}
					}

					Item** weaponToBreak = nullptr;
					ItemType weaponType = static_cast<ItemType>(WOODEN_SHIELD);
					bool hasMeleeGloves = false;
					if ( myStats->gloves && !shapeshifted )
					{
						switch ( myStats->gloves->type )
						{
						case BRASS_KNUCKLES:
						case IRON_KNUCKLES:
						case SPIKED_GAUNTLETS:
							hasMeleeGloves = true;
							break;
						default:
							break;
						}
					}
					if ( myStats->weapon && !shapeshifted )
					{
						weaponToBreak = &myStats->weapon;
					}
					else if ( hasMeleeGloves )
					{
						weaponToBreak = &myStats->gloves;
					}
					if ( weaponToBreak != nullptr && !shapeshifted )
					{
						weaponType = (*weaponToBreak)->type;
					}

					Sint32 oldHP = hitstats->HP;
					hit.entity->modHP(-damage); // do the damage

					bool skillIncreased = false;
					// skill increase
					// can raise skills up to skill level 20 on dummybots...
					bool doSkillIncrease = true;
					if ( monsterIsImmobileTurret(hit.entity, hitstats) )
					{
						if ( hitstats->type == DUMMYBOT && hitstats->HP > 0 )
						{
							doSkillIncrease = true; // can train on dummybots.
						}
						else
						{
							doSkillIncrease = false; // no skill for killing/hurting other turrets.
						}
					}
					if ( hit.entity->behavior == &actPlayer && behavior == &actPlayer )
					{
						doSkillIncrease = false; // no skill for killing/hurting players
					}
					if ( doSkillIncrease
						&& ((weaponskill >= PRO_SWORD && weaponskill <= PRO_POLEARM) || weaponskill == PRO_UNARMED || (whip && weaponskill == PRO_RANGED)) )
					{
						if ( myStats->weapon &&
							(myStats->weapon->type == CRYSTAL_BATTLEAXE
								|| myStats->weapon->type == CRYSTAL_MACE
								|| myStats->weapon->type == CRYSTAL_SWORD
								|| myStats->weapon->type == CRYSTAL_SPEAR) )
						{
							int chance = 6;
							bool notify = true;
							if ( myStats->type == GOBLIN )
							{
								chance = 10;
								notify = true;
							}

							if ( local_rng.rand() % chance == 0 )
							{
								if ( hitstats->type != DUMMYBOT || (hitstats->type == DUMMYBOT && myStats->getProficiency(weaponskill) < SKILL_LEVEL_BASIC) )
								{
									this->increaseSkill(weaponskill, notify);
									skillIncreased = true;
								}
							}
						}
						else if ( hitstats->HP <= 0 )
						{
							if ( player >= 0 && weaponskill == PRO_UNARMED 
								&& stats[player]->type == GOATMAN
								&& stats[player]->EFFECTS[EFF_DRUNK] )
							{
								steamStatisticUpdateClient(player, STEAM_STAT_BARFIGHT_CHAMP, STEAM_STAT_INT, 1);
							}
							int chance = 8;
							bool notify = true;
							if ( myStats->type == GOBLIN )
							{
								chance = 12;
								notify = true;
							}
							if ( local_rng.rand() % chance == 0 )
							{
								this->increaseSkill(weaponskill, notify);
								skillIncreased = true;
							}
						}
						else
						{
							int chance = 10;
							bool notify = true;
							if ( myStats->type == GOBLIN && weaponskill != PRO_RANGED )
							{
								chance = 14;
								notify = true;
							}
							if ( local_rng.rand() % chance == 0 )
							{
								if ( hitstats->type != DUMMYBOT || (hitstats->type == DUMMYBOT && myStats->getProficiency(weaponskill) < SKILL_LEVEL_BASIC) )
								{
									this->increaseSkill(weaponskill, notify);
									skillIncreased = true;
								}
							}
						}
					}

					if ( skillIncreased && myStats->type == GOBLIN && weaponskill != PRO_RANGED )
					{
						// goblins level up all combat skills at once.
						int numIncreases = 0;
						if ( weaponskill != PRO_SWORD )
						{
							numIncreases += this->increaseSkill(PRO_SWORD, false) ? 1 : 0;
						}
						if ( weaponskill != PRO_MACE )
						{
							numIncreases += this->increaseSkill(PRO_MACE, false) ? 1 : 0;
						}
						if ( weaponskill != PRO_AXE )
						{
							numIncreases += this->increaseSkill(PRO_AXE, false) ? 1 : 0;
						}
						if ( weaponskill != PRO_POLEARM )
						{
							numIncreases += this->increaseSkill(PRO_POLEARM, false) ? 1 : 0;
						}
						if ( weaponskill != PRO_UNARMED )
						{
							numIncreases += this->increaseSkill(PRO_UNARMED, false) ? 1 : 0;
						}
						if ( player >= 0 && numIncreases > 0 )
						{
							Uint32 color = makeColorRGB(255, 255, 0);
							messagePlayerColor(player, MESSAGE_PROGRESSION, color, Language::get(3446));
						}
					}

					// write the obituary
					if ( hit.entity != this )
					{
						killedByMonsterObituary(hit.entity);
					}

					// damage weapon if applicable

					bool isWeakWeapon = false;
					bool artifactWeapon = false;
					bool degradeWeapon = false;

					if ( weaponToBreak != nullptr && weaponType != WOODEN_SHIELD )
					{
						if ( behavior == &actPlayer )
						{
							Compendium_t::Events_t::eventUpdate(skill[2], Compendium_t::CPDM_ATTACKS,
								weaponType, 1);
							if ( pose == PLAYER_POSE_GOLEM_SMASH )
							{
								Compendium_t::Events_t::eventUpdate(skill[2], Compendium_t::CPDM_SPELL_DMG, SPELL_ITEM, damage, false, SPELL_STRIKE);
							}
							else
							{
								Compendium_t::Events_t::eventUpdate(skill[2], Compendium_t::CPDM_DMG_MAX, weaponType, damage);
							}
						}

						if ( weaponType == ARTIFACT_AXE || weaponType == ARTIFACT_MACE || weaponType == ARTIFACT_SPEAR || weaponType == ARTIFACT_SWORD )
						{
							artifactWeapon = true;
						}
						else if ( weaponType == CRYSTAL_BATTLEAXE || weaponType == CRYSTAL_MACE || weaponType == CRYSTAL_SWORD || weaponType == CRYSTAL_SPEAR )
						{
							// crystal weapons degrade faster.
							isWeakWeapon = true;
						}

						if ( myStats->type == GOBLIN )
						{
							isWeakWeapon = false;
						}

						if ( !artifactWeapon )
						{
							// normal weapons chance to not degrade 75% chance on 0 dmg, else 98%
							int degradeOnZeroDMG = 4 + (myStats->type == GOBLIN ? 4 : 0);
							int degradeOnNormalDMG = 50 + (myStats->type == GOBLIN ? 20 : 0);

							if ( !myStats->weapon && (*weaponToBreak) )
							{
								// unarmed glove weapons, 87.5% to not degrade on 0 dmg, else 99%
								degradeOnZeroDMG = 8 + (myStats->type == GOBLIN ? 4 : 0);
								degradeOnNormalDMG = 100 + (myStats->type == GOBLIN ? 20 : 0);
							}
							else if ( isWeakWeapon )
							{
								// crystal weapons chance to not degrade 66% chance on 0 dmg, else 97.5%
								degradeOnZeroDMG = 3;
								degradeOnNormalDMG = 40;
							}

							if ( behavior == &actPlayer && ((weaponskill >= PRO_SWORD && weaponskill <= PRO_POLEARM) 
								|| weaponskill == PRO_UNARMED || weaponskill == PRO_RANGED) )
							{
								int skillLVL = myStats->getModifiedProficiency(weaponskill) / 20;
								degradeOnZeroDMG += skillLVL; // increase by 1-5
								degradeOnNormalDMG += (skillLVL * 10); // increase by 10-50
							}
							if ( myStats->weapon && myStats->weapon->type == TOOL_WHIP )
							{
								degradeOnZeroDMG = degradeOnNormalDMG; // don't degrade faster on 0 dmg.
							}
							if ( behavior == &actPlayer && (svFlags & SV_FLAG_HARDCORE) )
							{
								// double durability.
								degradeOnZeroDMG *= 2;
								degradeOnNormalDMG *= 2;
							}

							if	( (local_rng.rand() % degradeOnZeroDMG == 0 && damage == 0)
									|| (local_rng.rand() % degradeOnNormalDMG == 0 && damage > 0)
								)
							{
								degradeWeapon = true;
							}

							if ( behavior == &actPlayer && skillCapstoneUnlocked(skill[2], weaponskill) )
							{
								// don't degrade on capstone skill.
								degradeWeapon = false;
							}
							else if ( myStats->type == SHADOW || myStats->type == LICH_FIRE || myStats->type == LICH_ICE )
							{
								degradeWeapon = false; //certain monster's weapons don't degrade.
							}
							else if ( myStats->type == SKELETON && behavior == &actMonster && monsterAllySummonRank != 0 )
							{
								degradeWeapon = false;
							}
							else if ( isIllusion )
							{
								degradeWeapon = false;
							}
							else if ( myStats->weapon && myStats->weapon->type == TOOL_WHIP )
							{
								degradeWeapon = false;
							}

							if ( degradeWeapon )
							{
								if ( (player >= 0 && players[player]->isLocalPlayer()) || player < 0 )
								{
									if ( (*weaponToBreak)->count > 1 )
									{
										newItem((*weaponToBreak)->type, (*weaponToBreak)->status, (*weaponToBreak)->beatitude,
											(*weaponToBreak)->count - 1, (*weaponToBreak)->appearance, (*weaponToBreak)->identified, &myStats->inventory);
									}
								}
								(*weaponToBreak)->count = 1;
								(*weaponToBreak)->status = static_cast<Status>((*weaponToBreak)->status - 1);
								if ( (*weaponToBreak)->status != BROKEN )
								{
									messagePlayer(player, MESSAGE_EQUIPMENT, Language::get(679));
									if ( behavior == &actPlayer )
									{
										Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_DEGRADED, (*weaponToBreak)->type, 1);
									}
								}
								else
								{
									playSoundEntity(this, 76, 64);
									messagePlayer(player, MESSAGE_EQUIPMENT, Language::get(680));
									if ( behavior == &actPlayer )
									{
										Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_BROKEN, (*weaponToBreak)->type, 1);
									}
								}
								if ( player > 0 && multiplayer == SERVER && !players[player]->isLocalPlayer() )
								{
									strcpy((char*)net_packet->data, "ARMR");
									if ( weaponToBreak == &myStats->weapon )
									{
										net_packet->data[4] = 5;
									}
									else
									{
										net_packet->data[4] = 2;
									}
									net_packet->data[5] = (*weaponToBreak)->status;
									net_packet->address.host = net_clients[player - 1].host;
									net_packet->address.port = net_clients[player - 1].port;
									net_packet->len = 6;
									sendPacketSafe(net_sock, -1, net_packet, player - 1);
								}
								if ( (*weaponToBreak)->status == BROKEN && behavior == &actMonster && playerhit >= 0 )
								{
									steamStatisticUpdateClient(playerhit, STEAM_STAT_TOUGH_AS_NAILS, STEAM_STAT_INT, 1);
								}
							}
						}
					}

					// damage opponent armor if applicable
					Item* armor = NULL;
					int armornum = 0;
					bool isWeakArmor = false;
					bool shieldIncreased = false;
					if ( damage > 0 || (damage == 0 && !(hitstats->shield && hitstats->defending)) )
					{
						// choose random piece of equipment to target
						armornum = hitstats->pickRandomEquippedItemToDegradeOnHit(&armor, true, false, false, true);
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

						int armorDegradeChance = 25;
						if ( isWeakArmor )
						{
							armorDegradeChance = 15;
							if ( weaponskill == PRO_MACE )
							{
								armorDegradeChance = 5;
							}
						}
						else
						{
							if ( weaponskill == PRO_MACE )
							{
								armorDegradeChance = 10;
							}
						}

						if ( hitstats->type == GOBLIN )
						{
							armorDegradeChance += 10;
						}

						if ( hit.entity->behavior == &actPlayer && armornum == 4 )
						{
							if ( skillCapstoneUnlocked(hit.entity->skill[2], PRO_SHIELD) )
							{
								armorDegradeChance = 100; // don't break.
							}
							else
							{
								if ( itemCategory(hitstats->shield) == ARMOR )
								{
									armorDegradeChance += 2 * (hitstats->getModifiedProficiency(PRO_SHIELD) / 10);
									armorDegradeChance += 10;
									if ( !players[hit.entity->skill[2]]->mechanics.itemDegradeRoll(hitstats->shield) )
									{
										armorDegradeChance = 100; // don't break.
									}
								}
								else
								{
									armorDegradeChance += (hitstats->getModifiedProficiency(PRO_SHIELD) / 10);
								}
							}
						}

						// crystal golem special attack increase chance for armor to break if hit. (25-33%)
						// special attack only degrades armor if primary target.
						if ( (pose == MONSTER_POSE_GOLEM_SMASH || pose == PLAYER_POSE_GOLEM_SMASH) && target == nullptr )
						{
							if ( isWeakArmor )
							{
								// 66% chance to be deselected from degrading.
								if ( local_rng.rand() % 3 > 0 )
								{
									armor = NULL;
									armornum = 0;
								}
							}
							else
							{
								// 75% chance to be deselected from degrading.
								if ( local_rng.rand() % 4 > 0 )
								{
									armor = NULL;
									armornum = 0;
								}
							}
						}
						else
						{
							if ( armorDegradeChance == 100 || (local_rng.rand() % armorDegradeChance > 0) )
							{
								armor = NULL;
								armornum = 0;
							}
						}
					}

					// if nothing chosen to degrade, check extra shield chances to degrade
					if ( hitstats->shield != NULL && hitstats->shield->status > BROKEN && armor == NULL
						&& !itemTypeIsQuiver(hitstats->shield->type) && itemCategory(hitstats->shield) != SPELLBOOK
						&& hitstats->shield->type != TOOL_TINKERING_KIT )
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
							if ( itemCategory(hitstats->shield) == ARMOR
								|| (hitstats->defending) )
							{
								int roll = 20;
								int hitskill = hitstats->getProficiency(PRO_SHIELD) / 20;
								roll += hitskill * 5;
								if ( damage == 0 )
								{
									roll /= 2;
								}
								if ( myStats->type == BAT_SMALL )
								{
									if ( hitstats->getProficiency(PRO_SHIELD) >= SKILL_LEVEL_BASIC )
									{
										roll *= 4;
									}
								}
								if ( roll > 0 )
								{
									bool success = (local_rng.rand() % roll == 0);
									if ( !success && playerhit >= 0 && hitstats->defending )
									{
  										if ( players[playerhit]->mechanics.defendTicks != 0 )
										{
											if ( (::ticks - players[playerhit]->mechanics.defendTicks) < (TICKS_PER_SECOND / 3) )
											{
												// perfect block timing, roll again
												success = (local_rng.rand() % roll == 0);
											}
										}
									}

									if ( success )
									{
										bool increaseSkill = true;
										if ( hit.entity->behavior == &actPlayer && behavior == &actPlayer )
										{
											increaseSkill = false;
										}
										else if ( hit.entity->behavior == &actPlayer && this->monsterAllyGetPlayerLeader() )
										{
											increaseSkill = false;
										}
										else if ( hit.entity->behavior == &actPlayer
											&& !players[hit.entity->skill[2]]->mechanics.allowedRaiseBlockingAgainstEntity(*this) )
										{
											increaseSkill = false;
										}
										else if ( hitstats->EFFECTS[EFF_SHAPESHIFT] )
										{
											increaseSkill = false;
										}
										else if ( itemCategory(hitstats->shield) != ARMOR
											&& hitstats->getProficiency(PRO_SHIELD) >= SKILL_LEVEL_SKILLED )
										{
											increaseSkill = false; // non-shield offhands dont increase skill past 40.
										}
										if ( increaseSkill )
										{
											hit.entity->increaseSkill(PRO_SHIELD); // increase shield skill
											shieldIncreased = true;
											if ( hit.entity->behavior == &actPlayer )
											{
												players[hit.entity->skill[2]]->mechanics.enemyRaisedBlockingAgainst[this->getUID()]++;
											}
										}
									}
								}
							}

							// shield still has chance to degrade after raising skill.
							// crystal golem special attack increase chance for shield to break if defended. (33%)
							// special attack only degrades shields if primary target.
							int shieldDegradeChance = 10;
							if ( svFlags & SV_FLAG_HARDCORE )
							{
								shieldDegradeChance = 40;
							}
							if ( hitstats->type == GOBLIN )
							{
								shieldDegradeChance += 10;
							}
							if ( weaponskill == PRO_MACE )
							{
								shieldDegradeChance *= 0.75;
							}
							if ( hit.entity->behavior == &actPlayer )
							{
								if ( itemCategory(hitstats->shield) == ARMOR )
								{
									shieldDegradeChance += 2 * (hitstats->getModifiedProficiency(PRO_SHIELD) / 10); // 2x shield bonus offhand
									if ( !players[hit.entity->skill[2]]->mechanics.itemDegradeRoll(hitstats->shield) )
									{
										shieldDegradeChance = 100; // don't break.
									}
								}
								else
								{
									shieldDegradeChance += (hitstats->getModifiedProficiency(PRO_SHIELD) / 10);
								}
								if ( skillCapstoneUnlocked(hit.entity->skill[2], PRO_SHIELD) )
								{
									shieldDegradeChance = 100; // don't break.
								}
							}
							if ( shieldDegradeChance < 100 && armor == NULL &&
								(	(hitstats->defending && local_rng.rand() % shieldDegradeChance == 0)
									|| (hitstats->defending && pose == MONSTER_POSE_GOLEM_SMASH && target == nullptr && local_rng.rand() % 3 == 0)
								)
								)
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
								steamStatisticUpdateClient(player, STEAM_STAT_UNSTOPPABLE_FORCE, STEAM_STAT_INT, 1);
								if ( armornum == 4 && hitstats->type == BUGBEAR 
									&& (hitstats->defending || hit.entity->monsterAttack == MONSTER_POSE_BUGBEAR_SHIELD) )
								{
									steamAchievementClient(player, "BARONY_ACH_BEAR_WITH_ME");
								}
							}
						}
					}

					bool statusInflicted = false;
					bool paralyzeStatusInflicted = false;
					bool slowStatusInflicted = false;
					bool bleedStatusInflicted = false;
					bool swordExtraDamageInflicted = false;
					bool knockbackInflicted = false;
					bool dyrnwynBurn = false;

					/*if ( thornsEffect < 0 )
					{
						hit.entity->modHP(thornsEffect);
					}*/

					// special weapon effects
					if ( myStats->weapon && !shapeshifted )
					{
						if ( myStats->weapon->type == ARTIFACT_SWORD )
						{
							real_t amount = 0.0;
							real_t percent = getArtifactWeaponEffectChance(myStats->weapon->type, *myStats, &amount);
							if ( dyrnwynSmite || (local_rng.rand() % 100 < static_cast<int>(percent)) )
							{
								if ( hit.entity->flags[BURNABLE] )
								{
									if ( hitstats )
									{
										hitstats->burningInflictedBy = static_cast<Sint32>(uid);
									}

									bool wasBurning = hit.entity->flags[BURNING];
									// Attempt to set the Entity on fire
									hit.entity->SetEntityOnFire();

									if ( !wasBurning && hit.entity->flags[BURNING] )
									{
										// 6 ticks maximum burning.
										hit.entity->char_fire = std::min(hit.entity->char_fire, static_cast<int>(TICKS_TO_PROCESS_FIRE * (6 + amount)));
										dyrnwynBurn = true;
									}

									// If a Player was hit, and they are now on fire, tell them what set them on fire
									if ( playerhit > 0 && hit.entity->flags[BURNING] )
									{
										messagePlayer(playerhit, MESSAGE_COMBAT, Language::get(683)); // "Dyrnwyn sets you on fire!"
									}
								}
							}
						}
						else if ( myStats->weapon->type == ARTIFACT_AXE && parashuProc )
						{
							int duration = 100; // 2 seconds
							if ( hitstats->HP > 0 && hit.entity->setEffect(EFF_SLOW, true, duration, true) )
							{
								slowStatusInflicted = true;
								playSoundEntity(hit.entity, 396 + local_rng.rand() % 3, 64);
								spawnMagicEffectParticles(hit.entity->x, hit.entity->y, hit.entity->z, 171);
							}
						}
					}

					if ( (hitstats->EFFECTS[EFF_WEBBED] || pose == PLAYER_POSE_GOLEM_SMASH) 
						&& !hitstats->EFFECTS[EFF_KNOCKBACK] && hit.entity->setEffect(EFF_KNOCKBACK, true, 30, false) )
					{
						real_t baseMultiplier = 0.7;
						if ( pose == PLAYER_POSE_GOLEM_SMASH )
						{
							baseMultiplier = 0.9;
						}
						real_t pushbackMultiplier = baseMultiplier;
						if ( !hit.entity->isMobile() )
						{
							pushbackMultiplier += 0.3;
						}
						real_t tangent = atan2(hit.entity->y - this->y, hit.entity->x - this->x);
						if ( hit.entity->behavior == &actMonster )
						{
							hit.entity->vel_x = cos(tangent) * pushbackMultiplier;
							hit.entity->vel_y = sin(tangent) * pushbackMultiplier;
							hit.entity->monsterKnockbackVelocity = 0.05;
							hit.entity->monsterKnockbackUID = this->getUID();
							hit.entity->monsterKnockbackTangentDir = tangent;
							hit.entity->lookAtEntity(*this);
							if ( !(backstab || flanking) )
							{
								if ( hit.entity->monsterAttack == 0 )
								{
									hit.entity->monsterHitTime = std::max(HITRATE - 12, hit.entity->monsterHitTime);
								}
							}
						}
						else if ( hit.entity->behavior == &actPlayer )
						{
							// normalize tangent
							while ( tangent < 0 )
							{
								tangent += 2 * PI;
							}
							while ( tangent > 2 * PI )
							{
								tangent -= 2 * PI;
							}
							if ( !players[hit.entity->skill[2]]->isLocalPlayer() )
							{
								hit.entity->monsterKnockbackVelocity = pushbackMultiplier;
								hit.entity->monsterKnockbackTangentDir = tangent;
								serverUpdateEntityFSkill(hit.entity, 11);
								serverUpdateEntityFSkill(hit.entity, 9);
							}
							else
							{
								hit.entity->monsterKnockbackVelocity = pushbackMultiplier;
								hit.entity->monsterKnockbackTangentDir = tangent;
							}
						}
						knockbackInflicted = true;
					}

					// player weapon skills
					if ( damage > 0 && weaponskill == PRO_UNARMED && behavior == &actPlayer && (charge >= MAXCHARGE - 3) )
					{
						int chance = 0;
						bool inflictParalyze = false;
						switch ( myStats->getModifiedProficiency(PRO_UNARMED) / 20 )
						{
							case 0:
								break;
							case 1:
								break;
							case 2:
								break;
							case 3:
								break;
							case 4:
								break;
							case 5:
								chance = 1;
								break;
							default:
								break;
						}
						if ( chance > 0 && backstab && !hitstats->EFFECTS[EFF_PARALYZED] && hitstats->HP > 0 )
						{
							int duration = 50;
							if ( hitstats->HP > 0 && hit.entity->setEffect(EFF_PARALYZED, true, duration, true) )
							{
								paralyzeStatusInflicted = true;
								playSoundEntity(hit.entity, 172, 64); //TODO: Paralyze spell sound.
								spawnMagicEffectParticles(hit.entity->x, hit.entity->y, hit.entity->z, 170);
								if ( behavior == &actPlayer ) // redundant; but if this code ever changes...
								{
									steamAchievementClient(skill[2], "BARONY_ACH_ONE_PUNCH_MAN");
									Compendium_t::Events_t::eventUpdateCodex(skill[2], Compendium_t::CPDM_SKILL_LEGENDARY_PROCS, "unarmed skill", 1);
								}
							}
							hit.entity->modHP(-5); // do extra damage.
						}
						if ( !knockbackInflicted && hasMeleeGloves
							&& hit.entity->setEffect(EFF_KNOCKBACK, true, 30, false) )
						{
							real_t baseMultiplier = 0.5;
							if ( myStats->gloves )
							{
								switch ( myStats->gloves->type )
								{
									case BRASS_KNUCKLES:
										baseMultiplier = 0.25;
										break;
									case IRON_KNUCKLES:
										baseMultiplier = 0.35;
										break;
									case SPIKED_GAUNTLETS:
										baseMultiplier = 0.5;
										break;
									default:
										break;
								}
							}
							real_t pushbackMultiplier = baseMultiplier + 0.1 * (myStats->getModifiedProficiency(PRO_UNARMED) / 20);
							/*if ( myStats->shield && hasMeleeGloves )
							{
								pushbackMultiplier /= 2;
							}*/
							if ( !hit.entity->isMobile() )
							{
								pushbackMultiplier += 0.3;
							}
							real_t tangent = atan2(hit.entity->y - this->y, hit.entity->x - this->x);
							if ( hit.entity->behavior == &actMonster )
							{
								hit.entity->vel_x = cos(tangent) * pushbackMultiplier;
								hit.entity->vel_y = sin(tangent) * pushbackMultiplier;
								hit.entity->monsterKnockbackVelocity = 0.05;
								hit.entity->monsterKnockbackUID = this->getUID();
								hit.entity->monsterKnockbackTangentDir = tangent;
								hit.entity->lookAtEntity(*this);
								if ( !(backstab || flanking) )
								{
									if ( hit.entity->monsterAttack == 0 )
									{
										hit.entity->monsterHitTime = std::max(HITRATE - 12, hit.entity->monsterHitTime);
									}
								}
							}
							else if ( hit.entity->behavior == &actPlayer )
							{
								// normalize tangent
								while ( tangent < 0 )
								{
									tangent += 2 * PI;
								}
								while ( tangent > 2 * PI )
								{
									tangent -= 2 * PI;
								}
								if ( !players[hit.entity->skill[2]]->isLocalPlayer() )
								{
									hit.entity->monsterKnockbackVelocity = pushbackMultiplier;
									hit.entity->monsterKnockbackTangentDir = tangent;
									serverUpdateEntityFSkill(hit.entity, 11);
									serverUpdateEntityFSkill(hit.entity, 9);
								}
								else
								{
									hit.entity->monsterKnockbackVelocity = pushbackMultiplier;
									hit.entity->monsterKnockbackTangentDir = tangent;
								}
							}
							knockbackInflicted = true;
						}
					}
					else if ( damage > 0 && behavior == &actPlayer && weaponskill >= PRO_SWORD && weaponskill <= PRO_POLEARM && (charge >= MAXCHARGE - 3) )
					{
						// special weapon effects.
						int capstoneDamage = 5;
						if ( weaponskill == PRO_AXE )
						{
							capstoneDamage = 10;
						}
						int chance = 0;
						switch ( myStats->getModifiedProficiency(weaponskill) / 20 )
						{
							case 0:
							case 1:
							case 2:
							case 3:
							case 4:
								break;
							case 5:
								chance = 4;
								break;
							default:
								break;
						}

						if ( weaponskill == PRO_POLEARM )
						{
							// knockback.
							if ( chance > 0 )
							{
								if ( hit.entity->behavior == &actMonster && hit.entity->setEffect(EFF_KNOCKBACK, true, 20, false) )
								{
									real_t pushbackMultiplier = 0.3 + 0.075 * (myStats->getModifiedProficiency(PRO_POLEARM) / 20);
									if ( !hit.entity->isMobile() )
									{
										pushbackMultiplier += 0.3;
									}
									real_t tangent = atan2(hit.entity->y - this->y, hit.entity->x - this->x);
									hit.entity->vel_x = cos(tangent) * pushbackMultiplier;
									hit.entity->vel_y = sin(tangent) * pushbackMultiplier;
									hit.entity->monsterKnockbackVelocity = 0.05;
									hit.entity->monsterKnockbackUID = this->getUID();
									hit.entity->monsterKnockbackTangentDir = tangent;
									hit.entity->lookAtEntity(*this);
									if ( !(backstab || flanking) )
									{
										if ( hit.entity->monsterAttack == 0 )
										{
											hit.entity->monsterHitTime = std::max(HITRATE - 12, hit.entity->monsterHitTime);
										}
									}
									knockbackInflicted = true;
									if ( behavior == &actPlayer )
									{
										Compendium_t::Events_t::eventUpdateCodex(skill[2], Compendium_t::CPDM_SKILL_LEGENDARY_PROCS, "polearm skill", 1);
									}
								}
								hit.entity->modHP(-capstoneDamage); // do the damage
							}
						}
						else if ( weaponskill == PRO_MACE && hitstats->HP > 0 )
						{
							// paralyze.
							if ( chance > 0 ) // chance based paralyze
							{
								if ( local_rng.rand() % chance == 0 && !hitstats->EFFECTS[EFF_PARALYZED] )
								{
									int duration = 75; // 1.5 seconds
									if ( hitstats->HP > 0 && hit.entity->setEffect(EFF_PARALYZED, true, duration, true) )
									{
										paralyzeStatusInflicted = true;
										playSoundEntity(hit.entity, 172, 64); //TODO: Paralyze spell sound.
										spawnMagicEffectParticles(hit.entity->x, hit.entity->y, hit.entity->z, 170);
										if ( behavior == &actPlayer )
										{
											Compendium_t::Events_t::eventUpdateCodex(skill[2], Compendium_t::CPDM_SKILL_LEGENDARY_PROCS, "mace skill", 1);
										}
									}
								}
								hit.entity->modHP(-capstoneDamage); // do the damage
							}
						}
						else if ( weaponskill == PRO_AXE && hitstats->HP > 0 )
						{
							// slow.
							if ( chance > 0 ) // always
							{
								int duration = 150; // 3 seconds
								if ( hitstats->HP > 0 && hit.entity->setEffect(EFF_SLOW, true, duration, true) && !slowStatusInflicted )
								{
									slowStatusInflicted = true;
									playSoundEntity(hit.entity, 396 + local_rng.rand() % 3, 64);
									spawnMagicEffectParticles(hit.entity->x, hit.entity->y, hit.entity->z, 171);
									if ( behavior == &actPlayer )
									{
										Compendium_t::Events_t::eventUpdateCodex(skill[2], Compendium_t::CPDM_SKILL_LEGENDARY_PROCS, "axe skill", 1);
									}
								}
								hit.entity->modHP(-capstoneDamage); // do the damage
								// don't re-notify if already inflicted slow from Parashu.
							}
						}
						else if ( weaponskill == PRO_SWORD && hitstats->HP > 0 )
						{
							// bleed.
							if ( chance > 0 ) // always
							{
								spawnMagicEffectParticles(hit.entity->x, hit.entity->y, hit.entity->z, 643);
								playSoundEntity(hit.entity, 173, 128);
								if ( gibtype[hitstats->type] > 0 && gibtype[(int)hitstats->type] != 5 )
								{
									bleedStatusInflicted = true;
									for ( int gibs = 0; gibs < 10; ++gibs )
									{
										Entity* gib = spawnGib(hit.entity);
										serverSpawnGibForClient(gib);
									}
									hit.entity->modHP(-capstoneDamage); // do the damage
									if ( behavior == &actPlayer )
									{
										Compendium_t::Events_t::eventUpdateCodex(skill[2], Compendium_t::CPDM_SKILL_LEGENDARY_PROCS, "sword skill", 1);
									}
								}
								else
								{
									swordExtraDamageInflicted = true;
									int extraDamage = 5;
									hit.entity->modHP(-(extraDamage + capstoneDamage)); // do the damage
								}
							}
						}
					}

					bool playerPoisonedTarget = false;

					// special monster effects
					if ( myStats->type == CRYSTALGOLEM && pose == MONSTER_POSE_GOLEM_SMASH )
					{
						if ( damage >= 150 && hit.entity->behavior == &actPlayer && playerhit >= 0 )
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
								serverSpawnMiscParticles(hit.entity, PARTICLE_EFFECT_ABILITY_ROCK, 78);
							}
							if ( target == nullptr )
							{
								// only play sound once on primary target.
								playSoundEntity(hit.entity, 181, 64);
							}
						}
					}
					else if ( myStats->type == BUGBEAR && pose == MONSTER_POSE_BUGBEAR_SHIELD )
					{
						if ( hit.entity->setEffect(EFF_KNOCKBACK, true, 30, false) )
						{
							real_t pushbackMultiplier = 0.9;
							knockbackInflicted = true;

							real_t tangent = atan2(hit.entity->y - this->y, hit.entity->x - this->x);
							if ( hit.entity->behavior == &actMonster )
							{
								hit.entity->vel_x = cos(tangent) * pushbackMultiplier;
								hit.entity->vel_y = sin(tangent) * pushbackMultiplier;
								hit.entity->monsterKnockbackVelocity = 0.01;
								hit.entity->monsterKnockbackUID = this->getUID();
								hit.entity->monsterKnockbackTangentDir = tangent;
								//hit.entity->lookAtEntity(*parent);
							}
							else if ( hit.entity->behavior == &actPlayer )
							{
								if ( !players[hit.entity->skill[2]]->isLocalPlayer() )
								{
									hit.entity->monsterKnockbackVelocity = pushbackMultiplier;
									hit.entity->monsterKnockbackTangentDir = tangent;
									serverUpdateEntityFSkill(hit.entity, 11);
									serverUpdateEntityFSkill(hit.entity, 9);
								}
								else
								{
									hit.entity->monsterKnockbackVelocity = pushbackMultiplier;
									hit.entity->monsterKnockbackTangentDir = tangent;
								}
							}
						}
					}
					else if ( myStats->type == BAT_SMALL )
					{
						if ( !hitstats->EFFECTS[EFF_BLEEDING] )
						{
							if ( !hitstats->defending )
							{
								if ( hit.entity->setEffect(EFF_BLEEDING, true, 6 * TICKS_PER_SECOND, false) )
								{
									statusInflicted = true;
									messagePlayer(playerhit, MESSAGE_COMBAT, Language::get(701));
								}
							}
						}
					}
					else if ( myStats->type == MIMIC && local_rng.rand() % 4 == 0 )
					{
						Item* armor = nullptr;
						int armornum = 0;
						if ( behavior == &actPlayer 
							|| (hit.entity->behavior == &actMonster
								&& ((hit.entity->monsterAllySummonRank != 0 && hitstats->type == SKELETON)
									|| hit.entity->monsterIsTinkeringCreation())) )
						{
							armor = nullptr;
						}
						else
						{
							if ( hitstats->defending && hitstats->shield && itemCategory(hitstats->shield) == ARMOR )
							{
								// try eat shield
								armornum = hitstats->pickRandomEquippedItem(&armor, true, false, true, true);
								if ( !armor )
								{
									armornum = hitstats->pickRandomEquippedItem(&armor, true, false, false, false);
								}
							}
							else
							{
								armornum = hitstats->pickRandomEquippedItem(&armor, true, false, false, false);
							}
						}
						if ( armor != nullptr )
						{
							int qty = 1;
							int startCount = armor->count;
							if ( itemTypeIsQuiver(armor->type) )
							{
								qty = armor->count;
								armor->count = 0;
							}
							else
							{
								armor->count--;
							}
							if ( hit.entity->behavior == &actPlayer && playerhit >= 0 )
							{
								steamStatisticUpdateClient(playerhit, STEAM_STAT_I_NEEDED_THAT, STEAM_STAT_INT, 1);
							}
							messagePlayer(playerhit, MESSAGE_COMBAT, Language::get(6085), armor->getName());
							Item* stolenArmor = newItem(armor->type, armor->status, armor->beatitude, qty, armor->appearance, armor->identified, &myStats->inventory);
							stolenArmor->ownerUid = hit.entity->getUID();
							stolenArmor->isDroppable = armor->isDroppable;
							if ( playerhit > 0 && multiplayer == SERVER && !players[playerhit]->isLocalPlayer() )
							{
								strcpy((char*)net_packet->data, "STLA");
								net_packet->data[4] = armornum;
								SDLNet_Write32(static_cast<Uint32>(armor->type), &net_packet->data[5]);
								SDLNet_Write32(static_cast<Uint32>(armor->status), &net_packet->data[9]);
								SDLNet_Write32(static_cast<Uint32>(armor->beatitude), &net_packet->data[13]);
								SDLNet_Write32(static_cast<Uint32>(startCount), &net_packet->data[17]);
								SDLNet_Write32(static_cast<Uint32>(armor->appearance), &net_packet->data[21]);
								net_packet->data[25] = armor->identified;
								net_packet->address.host = net_clients[playerhit - 1].host;
								net_packet->address.port = net_clients[playerhit - 1].port;
								net_packet->len = 26;
								sendPacketSafe(net_sock, -1, net_packet, playerhit - 1);
							}

							if ( armor->count <= 0 )
							{
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
							}
						}
					}
					else if ( (damage > 0 || hitstats->EFFECTS[EFF_PACIFY] || hitstats->EFFECTS[EFF_FEAR]) && local_rng.rand() % 4 == 0 )
					{
						switch ( myStats->type )
						{
						case SCORPION:
							hitstats->EFFECTS[EFF_PARALYZED] = true;
							hitstats->EFFECTS_TIMERS[EFF_PARALYZED] = std::max(50, 150 - hit.entity->getCON() * 5);
							messagePlayer(playerhit, MESSAGE_COMBAT, Language::get(684));
							messagePlayer(playerhit, MESSAGE_COMBAT, Language::get(685));
							serverUpdateEffects(playerhit);
							break;
						case SPIDER:
						{
							bool applyPoison = true;
							if ( behavior == &actPlayer )
							{
								if ( charge >= MAXCHARGE - 3 ) // fully charged strike injects venom.
								{
									applyPoison = true;
								}
								else
								{
									applyPoison = false;
								}
							}
							if ( applyPoison )
							{
								playerPoisonedTarget = true;
								hitstats->EFFECTS[EFF_POISONED] = true;
								hitstats->EFFECTS_TIMERS[EFF_POISONED] = std::max(200, 600 - hit.entity->getCON() * 20);
								hitstats->poisonKiller = getUID();
								if ( arachnophobia_filter ) {
									messagePlayer(playerhit, MESSAGE_COMBAT, Language::get(4089));
								}
								else {
									messagePlayer(playerhit, MESSAGE_COMBAT, Language::get(686));
								}
								messagePlayer(playerhit, MESSAGE_COMBAT, Language::get(687));
								serverUpdateEffects(playerhit);
								for ( int tmp = 0; tmp < 3; ++tmp )
								{
									Entity* gib = spawnGib(hit.entity, 211);
									serverSpawnGibForClient(gib);
								}
							}
							break;
						}
						case SUCCUBUS:
						{
							Item* armor = nullptr;
							int armornum = 0;
							if ( behavior == &actPlayer 
								|| (hit.entity->behavior == &actMonster
									&& ( (hit.entity->monsterAllySummonRank != 0 && hitstats->type == SKELETON)
									|| hit.entity->monsterIsTinkeringCreation())) )
							{
								armor = nullptr;
							}
							else
							{
								if ( !monsterAllyGetPlayerLeader() )
								{
									if ( currentlevel >= 10 )
									{
										armornum = hitstats->pickRandomEquippedItem(&armor, true, false, false, false);
									}
									else
									{
										if ( local_rng.rand() % 4 == 0 )
										{
											armornum = hitstats->pickRandomEquippedItem(&armor, true, false, false, false);
										}
									}
								}
								else
								{
									if ( local_rng.rand() % 8 == 0 )
									{
										armornum = hitstats->pickRandomEquippedItem(&armor, true, false, false, false);
									}
								}
							}
							if ( armor != nullptr )
							{
								int startCount = armor->count;
								int qty = 1;
								if ( itemTypeIsQuiver(armor->type) )
								{
									qty = armor->count;
									armor->count = 0;
								}
								else
								{
									armor->count--;
								}
								messagePlayer(playerhit, MESSAGE_COMBAT, Language::get(688), armor->getName());
								Item* stolenArmor = newItem(armor->type, armor->status, armor->beatitude, qty, armor->appearance, armor->identified, &myStats->inventory);
								stolenArmor->ownerUid = hit.entity->getUID();
								if ( playerhit > 0 && multiplayer == SERVER && !players[playerhit]->isLocalPlayer() )
								{
									strcpy((char*)net_packet->data, "STLA");
									net_packet->data[4] = armornum;
									SDLNet_Write32(static_cast<Uint32>(armor->type), &net_packet->data[5]);
									SDLNet_Write32(static_cast<Uint32>(armor->status), &net_packet->data[9]);
									SDLNet_Write32(static_cast<Uint32>(armor->beatitude), &net_packet->data[13]);
									SDLNet_Write32(static_cast<Uint32>(startCount), &net_packet->data[17]);
									SDLNet_Write32(static_cast<Uint32>(armor->appearance), &net_packet->data[21]);
									net_packet->data[25] = armor->identified;
									net_packet->address.host = net_clients[playerhit - 1].host;
									net_packet->address.port = net_clients[playerhit - 1].port;
									net_packet->len = 26;
									sendPacketSafe(net_sock, -1, net_packet, playerhit - 1);
								}

								if ( armor->count <= 0 )
								{
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
								}

								teleportRandom();

								// the succubus loses interest after this
								monsterState = 0;
								monsterTarget = 0;
							}
							break;
						}
						default:
							break;
						}
					}
					else if ( damage == 0 && !(hitstats->defending) )
					{
						// special chance effects when damage is 0.
						if ( local_rng.rand() % 20 == 0 )
						{
							switch ( myStats->type )
							{
								case SCORPION:
									hitstats->EFFECTS[EFF_PARALYZED] = true;
									hitstats->EFFECTS_TIMERS[EFF_PARALYZED] = std::max(50, 150 - hit.entity->getCON() * 5);
									messagePlayer(playerhit, MESSAGE_COMBAT, Language::get(684));
									messagePlayer(playerhit, MESSAGE_COMBAT, Language::get(685));
									serverUpdateEffects(playerhit);
									statusInflicted = true;
									break;
								case SPIDER:
									if ( behavior != &actPlayer )
									{
										hitstats->EFFECTS[EFF_POISONED] = true;
										hitstats->EFFECTS_TIMERS[EFF_POISONED] = std::max(200, 300 - hit.entity->getCON() * 20);
										if (arachnophobia_filter) {
										    messagePlayer(playerhit, MESSAGE_COMBAT, Language::get(4089));
										} else {
										    messagePlayer(playerhit, MESSAGE_COMBAT, Language::get(686));
										}
										messagePlayer(playerhit, MESSAGE_COMBAT, Language::get(687));
										serverUpdateEffects(playerhit);
										statusInflicted = true;
									}
									break;
								default:
									break;
							}
						}
					}

					if ( player >= 0 && hit.entity->behavior == &actMonster )
					{
						bool oldRhythmStatus = achievementStatusRhythmOfTheKnight[player];
						updateAchievementRhythmOfTheKnight(player, hit.entity, false);
						if ( !oldRhythmStatus && achievementStatusRhythmOfTheKnight[player] )
						{
							//messagePlayer(0, MESSAGE_DEBUG, "rhythm roll on atk");
							if ( local_rng.rand() % 10 < 8 )
							{
								bool increaseSkill = true;
								if ( this->behavior == &actPlayer )
								{
									if ( !players[this->skill[2]]->mechanics.allowedRaiseBlockingAgainstEntity(*hit.entity) )
									{
										increaseSkill = false;
									}
									if ( myStats->shield && itemCategory(myStats->shield) != ARMOR )
									{
										if ( myStats->getProficiency(PRO_SHIELD) >= SKILL_LEVEL_SKILLED )
										{
											increaseSkill = false;
										}
									}
									if ( increaseSkill )
									{
										players[this->skill[2]]->mechanics.enemyRaisedBlockingAgainst[hit.entity->getUID()]++;
									}
								}
								if ( increaseSkill )
								{
									this->increaseSkill(PRO_SHIELD);
								}
							}
							achievementStatusRhythmOfTheKnight[player] = false;
							achievementRhythmOfTheKnightVec[player].clear(); // reset for the next one
						}
					}

					bool artifactWeaponProc = parashuProc || dyrnwynSmite || dyrnwynBurn || gugnirProc;

					// send messages
					if ( !strcmp(hitstats->name, "") || monsterNameIsGeneric(*hitstats) )
					{
						Uint32 color = makeColorRGB(0, 255, 0);
						Uint32 colorSpecial = color;// makeColorRGB(255, 0, 255);
						if ( hitstats->HP > 0 )
						{
							if ( !artifactWeaponProc )
							{
								if ( damage > olddamage )
								{
									// critical hit
									messagePlayerMonsterEvent(player, color, *hitstats, Language::get(689), Language::get(689), MSG_COMBAT);
								}
								else
								{
									// normal hit
									messagePlayerMonsterEvent(player, color, *hitstats, Language::get(690), Language::get(690), MSG_COMBAT_BASIC);
								}
							}

							if ( dyrnwynSmite )
							{
								messagePlayerMonsterEvent(player, colorSpecial, *hitstats, Language::get(3754), Language::get(3755), MSG_COMBAT);
							}
							else if ( dyrnwynBurn )
							{
								messagePlayerMonsterEvent(player, colorSpecial, *hitstats, Language::get(3756), Language::get(3757), MSG_COMBAT);
							}
							else if ( parashuProc )
							{
								messagePlayerMonsterEvent(player, colorSpecial, *hitstats, Language::get(3758), Language::get(3759), MSG_COMBAT);
							}
							else if ( gugnirProc )
							{
								messagePlayerMonsterEvent(player, colorSpecial, *hitstats, Language::get(3760), Language::get(3761), MSG_COMBAT);
							}

							if ( damage == 0 )
							{
								// blow bounces off
								messagePlayer(player, MESSAGE_COMBAT_BASIC, Language::get(691));
							}
							else
							{
								if ( flanking )
								{
									// flank defenses
									messagePlayerMonsterEvent(player, color, *hitstats, Language::get(2545), Language::get(2545), MSG_COMBAT);
								}
								else if ( backstab )
								{
									// backstab on unaware enemy
									messagePlayerMonsterEvent(player, color, *hitstats, Language::get(2543), Language::get(2543), MSG_COMBAT);
									if ( player >= 0 && hitstats->EFFECTS[EFF_SHADOW_TAGGED] && this->creatureShadowTaggedThisUid == hit.entity->getUID() )
									{
										achievementObserver.awardAchievementIfActive(player, this, AchievementObserver::BARONY_ACH_OHAI_MARK);
									}
								}
							}

							if ( playerPoisonedTarget )
							{
								messagePlayerMonsterEvent(player, color, *hitstats, Language::get(3478), Language::get(3479), MSG_COMBAT);
							}
							if ( paralyzeStatusInflicted )
							{
								messagePlayerMonsterEvent(player, color, *hitstats, Language::get(3206), Language::get(3205), MSG_COMBAT);
							}
							else if ( slowStatusInflicted )
							{
								messagePlayerMonsterEvent(player, color, *hitstats, Language::get(394), Language::get(393), MSG_COMBAT);
							}
							else if ( swordExtraDamageInflicted )
							{
								messagePlayerMonsterEvent(player, color, *hitstats, Language::get(3211), Language::get(3210), MSG_COMBAT);
							}
							else if ( knockbackInflicted )
							{
								messagePlayerMonsterEvent(player, color, *hitstats, Language::get(3215), Language::get(3214), MSG_COMBAT);
							}
						}
						else
						{
							// HP <= 0
							if ( backstab )
							{
								// assassinate monster
								messagePlayerMonsterEvent(player, color, *hitstats, Language::get(2547), Language::get(2547), MSG_COMBAT);
								if ( hitstats->type == COCKATRICE )
								{
									steamAchievementClient(player, "BARONY_ACH_SCALES_IN_FAVOR");
								}
								if ( player >= 0 && stats[player]->type == VAMPIRE && isInvisible() )
								{
									steamStatisticUpdateClient(player, STEAM_STAT_BLOOD_SPORT, STEAM_STAT_INT, 1);
								}
								if ( player >= 0 && hitstats->EFFECTS[EFF_SHADOW_TAGGED] && this->creatureShadowTaggedThisUid == hit.entity->getUID() )
								{
									achievementObserver.awardAchievementIfActive(player, this, AchievementObserver::BARONY_ACH_OHAI_MARK);
								}
							}
							else
							{
								// kill monster
								messagePlayerMonsterEvent(player, color, *hitstats, Language::get(692), Language::get(692), MSG_COMBAT);
								if ( player >= 0 && hit.entity && hit.entity->behavior == &actMonster )
								{
									real_t hitAngle = hit.entity->yawDifferenceFromEntity(this);
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

							if ( player >= 0 )
							{
								Compendium_t::Events_t::eventUpdateCodex(player, Compendium_t::CPDM_MELEE_KILLS, "melee", 1);
								if ( chargeMult > 1 )
								{
									Compendium_t::Events_t::eventUpdateCodex(player, Compendium_t::CPDM_CRIT_KILLS, "crits", 1);
								}
								if ( backstab )
								{
									Compendium_t::Events_t::eventUpdateCodex(player, Compendium_t::CPDM_BACKSTAB_KILLS, "backstabs", 1);
									Compendium_t::Events_t::eventUpdateCodex(player, Compendium_t::CPDM_CLASS_BACKSTAB_KILLS_RUN, "backstabs", 1);
								}
							}

							if ( player >= 0 && myStats->weapon && this->checkEnemy(hit.entity) )
							{
								if ( myStats->weapon->ownerUid == hit.entity->getUID() )
								{
									achievementObserver.awardAchievementIfActive(player, hit.entity, AchievementObserver::BARONY_ACH_IRONIC_PUNISHMENT);
								}
								if ( myStats->weapon->type == TOOL_WHIP )
								{
									achievementObserver.awardAchievementIfActive(player, hit.entity, AchievementObserver::BARONY_ACH_COWBOY_FROM_HELL);
								}
								if ( weaponskill == PRO_AXE && client_classes[player] == CLASS_PUNISHER )
								{
									if ( hitstats->EFFECTS[EFF_DISORIENTED] || hitstats->EFFECTS[EFF_PARALYZED]
										|| hitstats->EFFECTS[EFF_SLOW] || hitstats->EFFECTS[EFF_ASLEEP] )
									{
										steamStatisticUpdateClient(player, STEAM_STAT_CHOPPING_BLOCK, STEAM_STAT_INT, 1);
									}
								}
							}
						}
					}
					else
					{
						Uint32 color = makeColorRGB(0, 255, 0);
						Uint32 colorSpecial = color;// makeColorRGB(255, 0, 255);
						if ( hitstats->HP > 0 )
						{
							if ( !artifactWeaponProc )
							{
								if ( damage > olddamage )
								{
									// critical hit
									messagePlayerMonsterEvent(player, color, *hitstats, Language::get(689), Language::get(693), MSG_COMBAT);
								}
								else
								{
									// normal hit
									messagePlayerMonsterEvent(player, color, *hitstats, Language::get(690), Language::get(694), MSG_COMBAT_BASIC);
								}
							}

							if ( dyrnwynSmite )
							{
								messagePlayerMonsterEvent(player, colorSpecial, *hitstats, Language::get(3754), Language::get(3755), MSG_COMBAT);
							}
							else if ( dyrnwynBurn )
							{
								messagePlayerMonsterEvent(player, colorSpecial, *hitstats, Language::get(3756), Language::get(3757), MSG_COMBAT);
							}
							else if ( parashuProc )
							{
								messagePlayerMonsterEvent(player, colorSpecial, *hitstats, Language::get(3758), Language::get(3759), MSG_COMBAT);
							}
							else if ( gugnirProc )
							{
								messagePlayerMonsterEvent(player, colorSpecial, *hitstats, Language::get(3760), Language::get(3761), MSG_COMBAT);
							}

							if ( damage == 0 )
							{
								// blow bounces off
								if ( hitstats->sex )
								{
									messagePlayerMonsterEvent(player, 0xFFFFFFFF, *hitstats, Language::get(691), Language::get(695), MSG_COMBAT_BASIC);
								}
								else
								{
									messagePlayerMonsterEvent(player, 0xFFFFFFFF, *hitstats, Language::get(691), Language::get(696), MSG_COMBAT_BASIC);
								}
							}
							else
							{
								if ( flanking )
								{
									// flank defenses
									messagePlayerMonsterEvent(player, color, *hitstats, Language::get(2545), Language::get(2546), MSG_COMBAT);
								}
								else if ( backstab )
								{
									// backstab on unaware enemy
									messagePlayerMonsterEvent(player, color, *hitstats, Language::get(2543), Language::get(2544), MSG_COMBAT);
									if ( player >= 0 && hitstats->EFFECTS[EFF_SHADOW_TAGGED] && this->creatureShadowTaggedThisUid == hit.entity->getUID() )
									{
										achievementObserver.awardAchievementIfActive(player, this, AchievementObserver::BARONY_ACH_OHAI_MARK);
									}
								}
							}

							if ( playerPoisonedTarget )
							{
								messagePlayerMonsterEvent(player, color, *hitstats, Language::get(3478), Language::get(3479), MSG_COMBAT);
							}
							if ( paralyzeStatusInflicted )
							{
								messagePlayerMonsterEvent(player, color, *hitstats, Language::get(3206), Language::get(3205), MSG_COMBAT);
							}
							else if ( slowStatusInflicted )
							{
								messagePlayerMonsterEvent(player, color, *hitstats, Language::get(394), Language::get(393), MSG_COMBAT);
							}
							else if ( swordExtraDamageInflicted )
							{
								messagePlayerMonsterEvent(player, color, *hitstats, Language::get(3211), Language::get(3210), MSG_COMBAT);
							}
							else if ( knockbackInflicted )
							{
								messagePlayerMonsterEvent(player, color, *hitstats, Language::get(3215), Language::get(3214), MSG_COMBAT);
							}
						}
						else
						{
							// HP <= 0
							if ( backstab )
							{
								// assassinate monster
								messagePlayerMonsterEvent(player, color, *hitstats, Language::get(2547), Language::get(2548), MSG_COMBAT);
								if ( hitstats->type == COCKATRICE )
								{
									steamAchievementClient(player, "BARONY_ACH_SCALES_IN_FAVOR");
								}
								if ( player >= 0 && stats[player]->type == VAMPIRE && isInvisible() )
								{
									steamStatisticUpdateClient(player, STEAM_STAT_BLOOD_SPORT, STEAM_STAT_INT, 1);
								}
								if ( player >= 0 && hitstats->EFFECTS[EFF_SHADOW_TAGGED] && this->creatureShadowTaggedThisUid == hit.entity->getUID() )
								{
									achievementObserver.awardAchievementIfActive(player, this, AchievementObserver::BARONY_ACH_OHAI_MARK);
								}
							}
							else
							{
								// kill monster
								messagePlayerMonsterEvent(player, color, *hitstats, Language::get(692), Language::get(697), MSG_COMBAT);
								if ( player >= 0 && hit.entity && hit.entity->behavior == &actMonster )
								{
									real_t hitAngle = hit.entity->yawDifferenceFromEntity(this);
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

							if ( player >= 0 )
							{
								Compendium_t::Events_t::eventUpdateCodex(player, Compendium_t::CPDM_MELEE_KILLS, "melee", 1);
								if ( chargeMult > 1 )
								{
									Compendium_t::Events_t::eventUpdateCodex(player, Compendium_t::CPDM_CRIT_KILLS, "crits", 1);
								}
							}

							if ( player >= 0 && myStats->weapon && this->checkEnemy(hit.entity) )
							{
								if ( myStats->weapon->ownerUid == hit.entity->getUID() )
								{
									achievementObserver.awardAchievementIfActive(player, hit.entity, AchievementObserver::BARONY_ACH_IRONIC_PUNISHMENT);
								}
								if ( myStats->weapon->type == TOOL_WHIP )
								{
									achievementObserver.awardAchievementIfActive(player, hit.entity, AchievementObserver::BARONY_ACH_COWBOY_FROM_HELL);
								}
								if ( weaponskill == PRO_AXE && client_classes[player] == CLASS_PUNISHER )
								{
									if ( hitstats->EFFECTS[EFF_DISORIENTED] || hitstats->EFFECTS[EFF_PARALYZED]
										|| hitstats->EFFECTS[EFF_SLOW] || hitstats->EFFECTS[EFF_ASLEEP] )
									{
										steamStatisticUpdateClient(player, STEAM_STAT_CHOPPING_BLOCK, STEAM_STAT_INT, 1);
									}
								}
							}
						}
					}

					bool disarmed = false;
					if ( hitstats->HP > 0 )
					{
						if ( !whip && hitstats->EFFECTS[EFF_DISORIENTED] )
						{
							hit.entity->setEffect(EFF_DISORIENTED, false, 0, false);

							// secondary alert to nerf the disorient time, second hit will aggro
							if ( myStats->mask && myStats->mask->type == MASK_PHANTOM && hit.entity->behavior == &actMonster )
							{
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
							}
						}
						else if ( whip && (hitstats->EFFECTS[EFF_DISORIENTED] 
							|| !hit.entity->isMobile()
							|| (hitstats->EFFECTS[EFF_DRUNK] && local_rng.rand() % 3 == 0)
							|| (hitstats->EFFECTS[EFF_CONFUSED] && local_rng.rand() % 3 == 0))
							)
						{
							if ( hit.entity->behavior == &actMonster && !hit.entity->isBossMonster() )
							{
								Uint32 color = makeColorRGB(0, 255, 0);
								if ( hitstats->weapon
									&& itemCategory(hitstats->weapon) != SPELLBOOK )
								{
									Entity* dropped = dropItemMonster(hitstats->weapon, hit.entity, hitstats);
									if ( dropped )
									{
										if ( hitstats->EFFECTS[EFF_DISORIENTED] && !hitstats->shield )
										{
											hit.entity->setEffect(EFF_DISORIENTED, false, 0, false);
										}
										playSoundEntity(hit.entity, 406, 128);
										dropped->itemDelayMonsterPickingUp = TICKS_PER_SECOND * 5;
										double tangent = atan2(hit.entity->y - y, hit.entity->x - x) + PI;
										dropped->yaw = tangent + PI;
										dropped->vel_x = (1.5 + .025 * (local_rng.rand() % 11)) * cos(tangent);
										dropped->vel_y = (1.5 + .025 * (local_rng.rand() % 11)) * sin(tangent);
										dropped->vel_z = (-10 - local_rng.rand() % 20) * .01;
										dropped->flags[USERFLAG1] = false;
										messagePlayerMonsterEvent(player, color, *hitstats, Language::get(3454), Language::get(3455), MSG_COMBAT);
										disarmed = true;
										dropped->itemOriginalOwner = hit.entity->getUID();
										if ( player >= 0 )
										{
											achievementObserver.addEntityAchievementTimer(hit.entity, AchievementObserver::BARONY_ACH_IRONIC_PUNISHMENT, -1, true, 0);
											achievementObserver.playerAchievements[player].ironicPunishmentTargets.insert(hit.entity->getUID());
										}
									}
								}
								else if ( hitstats->shield )
								{
									Entity* dropped = dropItemMonster(hitstats->shield, hit.entity, hitstats);
									if ( dropped )
									{
										if ( hitstats->EFFECTS[EFF_DISORIENTED] )
										{
											hit.entity->setEffect(EFF_DISORIENTED, false, 0, false);
										}
										playSoundEntity(hit.entity, 406, 128);
										dropped->itemDelayMonsterPickingUp = TICKS_PER_SECOND * 5;
										double tangent = atan2(hit.entity->y - y, hit.entity->x - x) + PI;
										dropped->yaw = tangent;
										dropped->vel_x = (1.5 + .025 * (local_rng.rand() % 11)) * cos(tangent);
										dropped->vel_y = (1.5 + .025 * (local_rng.rand() % 11)) * sin(tangent);
										dropped->vel_z = (-10 - local_rng.rand() % 20) * .01;
										dropped->flags[USERFLAG1] = false;
										messagePlayerMonsterEvent(player, color, *hitstats, Language::get(3456), Language::get(3457), MSG_COMBAT);
										disarmed = true;
										dropped->itemOriginalOwner = hit.entity->getUID();
									}
								}
								else
								{
									if ( hitstats->EFFECTS[EFF_DISORIENTED] )
									{
										hit.entity->setEffect(EFF_DISORIENTED, false, 0, false);
									}
								}
							}
							else
							{
								if ( hitstats->EFFECTS[EFF_DISORIENTED] )
								{
									hit.entity->setEffect(EFF_DISORIENTED, false, 0, false);
								}
							}
						}
						else if ( myStats->mask && myStats->mask->type == MASK_PHANTOM && backstab && hit.entity->behavior == &actMonster )
						{
							bool doPhantomStrike = false;
							if ( behavior == &actPlayer )
							{
								auto& tracker = achievementObserver.playerAchievements[skill[2]].phantomMaskFirstStrikes;
								if ( tracker.find(hit.entity->getUID()) == tracker.end() )
								{
									doPhantomStrike = true;
								}
							}

							if ( !hitstats->EFFECTS[EFF_DISORIENTED] && doPhantomStrike )
							{
								if ( hit.entity->setEffect(EFF_DISORIENTED, true, TICKS_PER_SECOND, false) )
								{
									achievementObserver.playerAchievements[skill[2]].phantomMaskFirstStrikes.insert(hit.entity->getUID());
									hit.entity->monsterReleaseAttackTarget();
									hit.entity->lookAtEntity(*this);
									hit.entity->monsterLookDir += (PI - PI / 4 + (local_rng.rand() % 10) * PI / 40);

									spawnFloatingSpriteMisc(134, hit.entity->x + (-4 + local_rng.rand() % 9) + cos(hit.entity->yaw) * 2,
										hit.entity->y + (-4 + local_rng.rand() % 9) + sin(hit.entity->yaw) * 2, hit.entity->z + local_rng.rand() % 4);
								}
							}
						}
					}

					if ( playerhit > 0 && multiplayer == SERVER && !players[playerhit]->isLocalPlayer() )
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
							strcpy((char*)net_packet->data, "SHAK");
							if ( damage > 0 )
							{
								net_packet->data[4] = 10; // turns into .1
								net_packet->data[5] = 10;
							}
							else
							{
								net_packet->data[4] = 5; // turns into .05
								net_packet->data[5] = 5;
							}
							net_packet->address.host = net_clients[playerhit - 1].host;
							net_packet->address.port = net_clients[playerhit - 1].port;
							net_packet->len = 6;
							sendPacketSafe(net_sock, -1, net_packet, playerhit - 1);
						}
					}
					else if ( playerhit == 0 || (splitscreen && playerhit > 0) )
					{
						if ( pose == MONSTER_POSE_GOLEM_SMASH || pose == PLAYER_POSE_GOLEM_SMASH )
						{
							if ( target == nullptr )
							{
								// primary target
								cameravars[playerhit].shakex += .2;
								cameravars[playerhit].shakey += 20;
							}
							else
							{
								// secondary target
								cameravars[playerhit].shakex += .1;
								cameravars[playerhit].shakey += 10;
							}
						}
						else if ( damage > 0 )
						{
							cameravars[playerhit].shakex += .1;
							cameravars[playerhit].shakey += 10;
						}
						else
						{
							cameravars[playerhit].shakex += .05;
							cameravars[playerhit].shakey += 5;
						}
					}

					if ( damage > 0 )
					{
						Entity* gib = spawnGib(hit.entity);
						serverSpawnGibForClient(gib);
						Uint32 color = makeColorRGB(255, 0, 0);
						messagePlayerMonsterEvent(playerhit, color, *myStats, Language::get(698), Language::get(699), MSG_ATTACKS);
						if ( playerhit >= 0 )
						{
							if ( behavior == &actMonster )
							{
								updateAchievementBaitAndSwitch(playerhit, false);
							}
							//messagePlayer(0, "took damage!");
							if ( paralyzeStatusInflicted )
							{
								messagePlayerMonsterEvent(playerhit, color, *myStats, Language::get(3208), Language::get(3207), MSG_COMBAT);
							}
							else if ( slowStatusInflicted )
							{
								messagePlayerMonsterEvent(playerhit, color, *myStats, Language::get(395), Language::get(395), MSG_COMBAT);
							}
							else if ( swordExtraDamageInflicted )
							{
								messagePlayerMonsterEvent(playerhit, color, *myStats, Language::get(3213), Language::get(3212), MSG_COMBAT);
							}
							else if ( knockbackInflicted )
							{
								messagePlayerMonsterEvent(playerhit, color, *myStats, Language::get(3216), Language::get(3216), MSG_COMBAT);
							}
						}
					}
					else
					{
						// display 'blow bounces off' message
						if ( !statusInflicted )
						{
							messagePlayerMonsterEvent(playerhit, 0xFFFFFFFF, *myStats, Language::get(2457), Language::get(2458), MSG_COMBAT_BASIC);
						}
						if ( myStats->type == COCKATRICE && hitstats->defending )
						{
							steamAchievementClient(playerhit, "BARONY_ACH_COCK_BLOCK");
						}
						else if ( myStats->type == MINOTAUR && !hitstats->defending )
						{
							steamAchievementClient(playerhit, "BARONY_ACH_ONE_WHO_KNOCKS");
						}
					}

					if ( playerhit >= 0 )
					{
						if ( hitstats->defending )
						{
							bool oldRhythmStatus = achievementStatusRhythmOfTheKnight[playerhit];
							updateAchievementRhythmOfTheKnight(playerhit, this, true);
							if ( !oldRhythmStatus && achievementStatusRhythmOfTheKnight[playerhit] )
							{
								if ( !shieldIncreased )
								{
									//messagePlayer(0, MESSAGE_DEBUG, "rhythm roll on hit");
									if ( local_rng.rand() % 10 < 8 )
									{
										bool skillIncrease = true;
										if ( hit.entity->behavior == &actPlayer )
										{
											if ( !players[hit.entity->skill[2]]->mechanics.allowedRaiseBlockingAgainstEntity(*this) )
											{
												skillIncrease = false;
											}
											if ( hitstats->shield && itemCategory(hitstats->shield) != ARMOR )
											{
												if ( hitstats->getProficiency(PRO_SHIELD) >= SKILL_LEVEL_SKILLED )
												{
													skillIncrease = false;
												}
											}
											if ( skillIncrease )
											{
												players[hit.entity->skill[2]]->mechanics.enemyRaisedBlockingAgainst[this->getUID()]++;
											}
										}
										if ( skillIncrease )
										{
											hit.entity->increaseSkill(PRO_SHIELD);
											shieldIncreased = true;
										}
									}
								}
								achievementStatusRhythmOfTheKnight[playerhit] = false;
								achievementRhythmOfTheKnightVec[playerhit].clear(); // reset for the next one
							}
							updateAchievementThankTheTank(playerhit, this, false);
						}
						else
						{
							achievementStatusRhythmOfTheKnight[playerhit] = false;
							achievementRhythmOfTheKnightVec[playerhit].clear();
							achievementThankTheTankPair[playerhit].erase(this->getUID());
							//messagePlayer(0, "used AC!");
						}
					}

					if ( playerhit >= 0 )
					{
						Compendium_t::Events_t::eventUpdateCodex(playerhit, Compendium_t::CPDM_HP_MOST_DMG_LOST_ONE_HIT, "hp", oldHP - hitstats->HP);
					}
					if ( player >= 0 )
					{
						if ( hitstats->HP < oldHP )
						{
							if ( weaponType != WOODEN_SHIELD )
							{
								Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_MELEE_DMG_TOTAL, weaponType, oldHP - hitstats->HP);
							}
							Compendium_t::Events_t::eventUpdateCodex(player, Compendium_t::CPDM_MELEE_DMG_TOTAL, "melee", oldHP - hitstats->HP);
							Compendium_t::Events_t::eventUpdateCodex(player, Compendium_t::CPDM_MELEE_HITS, "melee", 1);
							Compendium_t::Events_t::eventUpdateCodex(player, Compendium_t::CPDM_CLASS_MELEE_HITS_RUN, "melee", 1);
							if ( chargeMult > 1 )
							{
								Compendium_t::Events_t::eventUpdateCodex(player, Compendium_t::CPDM_CRITS_DMG_TOTAL, "crits", oldHP - hitstats->HP);
								Compendium_t::Events_t::eventUpdateCodex(player, Compendium_t::CPDM_CRIT_HITS, "crits", 1);
								Compendium_t::Events_t::eventUpdateCodex(player, Compendium_t::CPDM_CLASS_CRITS_HITS_RUN, "crits", 1);
							}
							if ( flanking )
							{
								Compendium_t::Events_t::eventUpdateCodex(player, Compendium_t::CPDM_FLANK_DMG, "flanking", oldHP - hitstats->HP);
								Compendium_t::Events_t::eventUpdateCodex(player, Compendium_t::CPDM_FLANK_HITS, "flanking", 1);
								Compendium_t::Events_t::eventUpdateCodex(player, Compendium_t::CPDM_CLASS_FLANK_HITS_RUN, "flanking", 1);
								Compendium_t::Events_t::eventUpdateCodex(player, Compendium_t::CPDM_CLASS_FLANK_DMG_RUN, "flanking", oldHP - hitstats->HP);
							}
							else if ( backstab )
							{
								Compendium_t::Events_t::eventUpdateCodex(player, Compendium_t::CPDM_BACKSTAB_HITS, "backstabs", 1);
								Compendium_t::Events_t::eventUpdateCodex(player, Compendium_t::CPDM_CLASS_BACKSTAB_HITS_RUN, "backstabs", 1);
								Compendium_t::Events_t::eventUpdateCodex(player, Compendium_t::CPDM_CLASS_BACKSTAB_DMG_RUN, "backstabs", oldHP - hitstats->HP);
							}
						}
					}

					if ( !strncmp(hitstats->name, "inner demon", strlen("inner demon")) )
					{
						hit.entity->modHP(damage); // undo melee damage.
					}

					DamageGib dmgGib = DMG_DEFAULT;
					bool charged = std::max(charge, MAXCHARGE / 2) / ((double)(MAXCHARGE / 2)) > 1;
					if ( weaponMultipliers >= 1.15 || (weaponskill == PRO_AXE && hitstats->type == MIMIC) )
					{
						dmgGib = DMG_STRONGER;
						if ( charged )
						{
							dmgGib = DMG_STRONGEST;
						}
					}
					else if ( charged )
					{
						dmgGib = DMG_STRONGER;
					}
					else if ( weaponMultipliers <= 0.85 )
					{
						dmgGib = DMG_WEAKER;
					}
					// update enemy bar for attacker
					if ( !strcmp(hitstats->name, "") )
					{
						updateEnemyBar(this, hit.entity, getMonsterLocalizedName(hitstats->type).c_str(), hitstats->HP, hitstats->MAXHP, false,
							dmgGib);
					}
					else
					{
						updateEnemyBar(this, hit.entity, hitstats->name, hitstats->HP, hitstats->MAXHP, false,
							dmgGib);
					}

					if ( thornsEffect != 0 && damage > 0 )
					{
						this->modHP(-abs(thornsEffect));
						if ( myStats->HP <= 0 && myStats->OLDHP > myStats->HP )
						{
							hit.entity->awardXP(this, true, true);
						}
						if ( player > 0 && multiplayer == SERVER && !players[player]->isLocalPlayer() )
						{
							strcpy((char*)net_packet->data, "SHAK");
							net_packet->data[4] = 10; // turns into .1
							net_packet->data[5] = 10;
							net_packet->address.host = net_clients[player - 1].host;
							net_packet->address.port = net_clients[player - 1].port;
							net_packet->len = 6;
							sendPacketSafe(net_sock, -1, net_packet, player - 1);
						}
						else if ( player >= 0 && players[player]->isLocalPlayer() )
						{
							cameravars[player].shakex += 0.1;
							cameravars[player].shakey += 10;
						}

						if ( player >= 0 )
						{
							Uint32 color = makeColorRGB(255, 0, 0);
							const char* thornsMsg = Language::get(6264); // named
							if ( !strcmp(hitstats->name, "") || monsterNameIsGeneric(*hitstats) )
							{
								thornsMsg = Language::get(6263);
							}

							if ( !strcmp(hitstats->name, "") )
							{
								messagePlayerColor(player, MESSAGE_COMBAT, color, thornsMsg, getMonsterLocalizedName(hitstats->type).c_str());
							}
							else
							{
								messagePlayerColor(player, MESSAGE_COMBAT, color, thornsMsg, hitstats->name);
							}
						}

						// update enemy bar for attacker
						if ( !strcmp(myStats->name, "") )
						{
							updateEnemyBar(hit.entity, this, getMonsterLocalizedName(myStats->type).c_str(), myStats->HP, myStats->MAXHP, false,
								DamageGib::DMG_DEFAULT);
						}
						else
						{
							updateEnemyBar(hit.entity, this, myStats->name, myStats->HP, myStats->MAXHP, false,
								DamageGib::DMG_DEFAULT);
						}
					}
					if ( hitstats->type == INCUBUS 
						&& !strncmp(hitstats->name, "inner demon", strlen("inner demon")) )
					{
						// conjuration deals damage back to attacker.
						Entity* illusionParent = uidToEntity(hit.entity->parent);
						this->modHP(-(std::max(2, damage / 2)) );
						playSoundEntity(this, 173, 64);
						if ( illusionParent )
						{
							if ( myStats->HP <= 0 )
							{
								illusionParent->awardXP(this, true, true);
								if ( illusionParent->behavior == &actPlayer )
								{
									steamStatisticUpdateClient(illusionParent->skill[2], STEAM_STAT_SELF_FLAGELLATION, STEAM_STAT_INT, 1);
								}
							}
							if ( player > 0 && multiplayer == SERVER && !players[player]->isLocalPlayer() )
							{
								strcpy((char*)net_packet->data, "SHAK");
								net_packet->data[4] = 10; // turns into .1
								net_packet->data[5] = 10;
								net_packet->address.host = net_clients[player - 1].host;
								net_packet->address.port = net_clients[player - 1].port;
								net_packet->len = 6;
								sendPacketSafe(net_sock, -1, net_packet, player - 1);
							}
							else if ( player >= 0 && players[player]->isLocalPlayer() )
							{
								cameravars[player].shakex += 0.1;
								cameravars[player].shakey += 10;
							}

							spawnMagicEffectParticles(this->x, this->y, this->z, 983);
							if ( illusionParent->behavior == &actPlayer && illusionParent != this )
							{
								// update enemy bar for attacker
								if ( !strcmp(myStats->name, "") )
								{
									updateEnemyBar(illusionParent, this, getMonsterLocalizedName(myStats->type).c_str(), myStats->HP, myStats->MAXHP, false,
										DamageGib::DMG_DEFAULT);
								}
								else
								{
									updateEnemyBar(illusionParent, this, myStats->name, myStats->HP, myStats->MAXHP, false,
										DamageGib::DMG_DEFAULT);
								}
							}
						}
					}

					if ( !disarmed )
					{
						if ( whip )
						{
							playSoundEntity(hit.entity, 407 + local_rng.rand() % 3, 64);
						}
						else
						{
							playSoundEntity(hit.entity, 28, 64);
						}
					}

					// chance of bleeding
					bool wasBleeding = hitstats->EFFECTS[EFF_BLEEDING]; // check if currently bleeding when this roll occurred.
					if ( gibtype[(int)hitstats->type] > 0 && gibtype[(int)hitstats->type] != 5 )
					{
						if ( bleedStatusInflicted || (hitstats->HP > 5 && damage > 0) )
						{
							if ( bleedStatusInflicted || (local_rng.rand() % 20 == 0 && (weaponskill > PRO_SWORD && weaponskill <= PRO_POLEARM) )
								|| (local_rng.rand() % 10 == 0 && weaponskill == PRO_SWORD)
								|| (whip && ( (flanking && local_rng.rand() % 5 == 0) || (backstab && local_rng.rand() % 2 == 0) || disarmed) )
								|| (local_rng.rand() % 4 == 0 && pose == MONSTER_POSE_GOLEM_SMASH)
								|| (local_rng.rand() % 4 == 0 && pose == PLAYER_POSE_GOLEM_SMASH)
								|| (thornsEffect < 0 && behavior == &actPlayer)
								|| (local_rng.rand() % 10 == 0 && myStats->type == VAMPIRE && myStats->weapon == nullptr)
								|| (local_rng.rand() % 8 == 0 && myStats->EFFECTS[EFF_VAMPIRICAURA] && (myStats->weapon == nullptr || myStats->type == LICH_FIRE))
							)
							{
								bool heavyBleedEffect = false; // heavy bleed will have a greater starting duration, and add to existing duration.
								if ( pose == MONSTER_POSE_GOLEM_SMASH )
								{
									heavyBleedEffect = true;
								}
								else if ( bleedStatusInflicted )
								{
									heavyBleedEffect = false;
								}
								else if ( (myStats->type == VAMPIRE && this->behavior == &actMonster) || myStats->EFFECTS[EFF_VAMPIRICAURA] )
								{
									if ( local_rng.rand() % 2 == 0 ) // 50% for heavy bleed effect.
									{
										heavyBleedEffect = true;
									}
								}

								char playerHitMessage[1024] = "";
								char monsterHitMessage[1024] = "";

								if ( (!wasBleeding && !heavyBleedEffect) || bleedStatusInflicted )
								{
									// normal bleed effect
									if ( bleedStatusInflicted ) // from sword capstone
									{
										// 5 seconds bleeding minimum
										hitstats->EFFECTS_TIMERS[EFF_BLEEDING] = std::max(hitstats->EFFECTS_TIMERS[EFF_BLEEDING], 250); 
									}
									else if ( myStats->weapon && myStats->weapon->type == TOOL_WHIP )
									{
										// 5 seconds bleeding minimum
										hitstats->EFFECTS_TIMERS[EFF_BLEEDING] = std::max(hitstats->EFFECTS_TIMERS[EFF_BLEEDING], 250);
										spawnMagicEffectParticles(hit.entity->x, hit.entity->y, hit.entity->z, 643);
										for ( int gibs = 0; gibs < 5; ++gibs )
										{
											Entity* gib = spawnGib(hit.entity);
											serverSpawnGibForClient(gib);
										}
									}
									else
									{
										hitstats->EFFECTS_TIMERS[EFF_BLEEDING] = std::max(480 + (int)local_rng.rand() % 360 - hit.entity->getCON() * 100, 120); // 2.4-16.8 seconds
									}
									hitstats->EFFECTS[EFF_BLEEDING] = true;
									strcpy(playerHitMessage, Language::get(701));
									if ( !strcmp(hitstats->name, "") || monsterNameIsGeneric(*hitstats) )
									{
										strcpy(monsterHitMessage, Language::get(702));
									}
									else
									{
										strcpy(monsterHitMessage, Language::get(703));
									}
								}
								else if ( heavyBleedEffect )
								{
									if ( !wasBleeding )
									{
										hitstats->EFFECTS_TIMERS[EFF_BLEEDING] = std::max(500 + (int)local_rng.rand() % 500 - hit.entity->getCON() * 10, 250); // 5-20 seconds
										hitstats->EFFECTS[EFF_BLEEDING] = true;
										strcpy(playerHitMessage, Language::get(2451));
										if ( !strcmp(hitstats->name, "") || monsterNameIsGeneric(*hitstats) )
										{
											strcpy(monsterHitMessage, Language::get(2452));
										}
										else
										{
											strcpy(monsterHitMessage, Language::get(2453));
										}
									}
									else
									{
										hitstats->EFFECTS_TIMERS[EFF_BLEEDING] += std::max((int)local_rng.rand() % 350 - hit.entity->getCON() * 5, 100); // 2-7 seconds in addition
										hitstats->EFFECTS[EFF_BLEEDING] = true;
										strcpy(playerHitMessage, Language::get(2454));
										if ( !strcmp(hitstats->name, "") || monsterNameIsGeneric(*hitstats) )
										{
											strcpy(monsterHitMessage, Language::get(2455));
										}
										else
										{
											strcpy(monsterHitMessage, Language::get(2456));
										}
									}
								}

								// message player of effect, skip if hit entity was already bleeding.
								if ( hitstats->EFFECTS[EFF_BLEEDING] && (!wasBleeding || heavyBleedEffect) )
								{
									hitstats->bleedInflictedBy = static_cast<Sint32>(this->getUID());
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
										Uint32 color = makeColorRGB(255, 0, 0);
										messagePlayerColor(playerhit, MESSAGE_STATUS, color, playerHitMessage);
									}
									else
									{
										Uint32 color = makeColorRGB(0, 255, 0);
										if ( !strcmp(hitstats->name, "") )
										{
											messagePlayerColor(player, MESSAGE_COMBAT, color, monsterHitMessage, getMonsterLocalizedName(hitstats->type).c_str());
										}
										else
										{
											messagePlayerColor(player, MESSAGE_COMBAT, color, monsterHitMessage, hitstats->name);
										}
									}

									// energize if wearing punisher hood!
									if ( myStats->helmet && myStats->helmet->type == PUNISHER_HOOD )
									{
										this->modMP(1 + local_rng.rand() % 2);
										Uint32 color = makeColorRGB(0, 255, 0);
										this->setEffect(EFF_MP_REGEN, true, 250, true);
										if ( behavior == &actPlayer )
										{
											messagePlayerColor(player, MESSAGE_HINT, color, Language::get(3753));
											steamStatisticUpdateClient(player, STEAM_STAT_ITS_A_LIVING, STEAM_STAT_INT, 1);
										}
										playSoundEntity(this, 168, 128);
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
								if ( playerhit > 0 && multiplayer == SERVER && !players[playerhit]->isLocalPlayer() )
								{
									strcpy((char*)net_packet->data, "SHAK");
									net_packet->data[4] = 10; // turns into .1
									net_packet->data[5] = 10;
									net_packet->address.host = net_clients[playerhit - 1].host;
									net_packet->address.port = net_clients[playerhit - 1].port;
									net_packet->len = 6;
									sendPacketSafe(net_sock, -1, net_packet, playerhit - 1);
								}
								else if ( playerhit >= 0 && players[playerhit]->isLocalPlayer() )
								{
									cameravars[playerhit].shakex += 0.1;
									cameravars[playerhit].shakey += 10;
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
										int explodeDmg = (40 + myStats->HP) * Entity::getDamageTableMultiplier(tmpEntity, *tmpStats, DAMAGE_TABLE_MAGIC); // check base magic damage resist.
										Entity* gib = spawnGib(tmpEntity);
										serverSpawnGibForClient(gib);
										playerhit = tmpEntity->skill[2];
										if ( playerhit > 0 && multiplayer == SERVER && !players[playerhit]->isLocalPlayer() )
										{
											strcpy((char*)net_packet->data, "SHAK");
											net_packet->data[4] = 20; // turns into .1
											net_packet->data[5] = 20;
											net_packet->address.host = net_clients[playerhit - 1].host;
											net_packet->address.port = net_clients[playerhit - 1].port;
											net_packet->len = 6;
											sendPacketSafe(net_sock, -1, net_packet, playerhit - 1);
										}
										else if ( playerhit >= 0 && players[playerhit]->isLocalPlayer() )
										{
											cameravars[playerhit].shakex += 0.2;
											cameravars[playerhit].shakey += 20;
										}
										tmpEntity->modHP(-explodeDmg);
										if ( playerhit >= 0 )
										{
											Uint32 color = makeColorRGB(255, 0, 0);
											messagePlayerColor(playerhit, MESSAGE_STATUS, color, Language::get(2523));
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
					bool tryLifesteal = false;
					bool forceLifesteal = false;
					int lifeStealAmount = damage;
					if ( damage > 0 )
					{
						if ( behavior == &actPlayer )
						{
							if ( myStats->weapon == nullptr || shapeshifted )
							{
								if ( myStats->EFFECTS_TIMERS[EFF_VAMPIRICAURA] == -2 )
								{
									if ( backstab || flanking )
									{
										if ( hitstats->HP <= 0 )
										{
											forceLifesteal = true;
										}
									}
								}
								else if ( myStats->EFFECTS[EFF_VAMPIRICAURA] && myStats->EFFECTS_TIMERS[EFF_VAMPIRICAURA] > 0 )
								{
									tryLifesteal = true;
									if ( backstab || flanking )
									{
										if ( hitstats->HP <= 0 )
										{
											forceLifesteal = true;
										}
									}
								}
								lifeStealAmount = std::max(0, hitstats->OLDHP - hitstats->HP);
								lifeStealAmount /= 4;
								lifeStealAmount = std::max(3, lifeStealAmount);
							}
						}
						else if ( (myStats->EFFECTS[EFF_VAMPIRICAURA] && (myStats->weapon == nullptr || myStats->type == LICH_FIRE)) )
						{
							tryLifesteal = true;
						}
						else if ( myStats->type == VAMPIRE && behavior == &actMonster )
						{
							tryLifesteal = true;
						}

						// special strike spell animation
						if ( pose == PLAYER_POSE_GOLEM_SMASH )
						{
							spawnMagicEffectParticles(hit.entity->x, hit.entity->y, hit.entity->z, 643);
							for ( int gibs = 0; gibs < 10; ++gibs )
							{
								Entity* gib = spawnGib(hit.entity);
								serverSpawnGibForClient(gib);
							}
							playSoundEntity(hit.entity, 181, 128);
						}
					}

					if ( tryLifesteal || forceLifesteal )
					{
						bool lifestealSuccess = false;
						if ( forceLifesteal )
						{
							this->modHP(lifeStealAmount);
							spawnMagicEffectParticles(x, y, z, 169);
							playSoundEntity(this, 168, 128);
							lifestealSuccess = true;
						}
						else if ( !wasBleeding && hitstats->EFFECTS[EFF_BLEEDING] )
						{
							// attack caused the target to bleed, trigger lifesteal tick
							this->modHP(lifeStealAmount);
							spawnMagicEffectParticles(x, y, z, 169);
							playSoundEntity(this, 168, 128);
							lifestealSuccess = true;
						}
						else if ( (local_rng.rand() % 4 == 0) && (myStats->type == VAMPIRE && behavior == &actMonster && myStats->EFFECTS[EFF_VAMPIRICAURA]) )
						{
							// vampires under aura have higher chance.
							this->modHP(lifeStealAmount);
							spawnMagicEffectParticles(x, y, z, 169);
							playSoundEntity(this, 168, 128);
							lifestealSuccess = true;
						}
						else if ( local_rng.rand() % 8 == 0 )
						{
							// else low chance for lifesteal.
							this->modHP(lifeStealAmount);
							spawnMagicEffectParticles(x, y, z, 169);
							playSoundEntity(this, 168, 128);
							lifestealSuccess = true;
						}

						if ( lifestealSuccess )
						{
							if ( player >= 0 )
							{
								myStats->HUNGER = std::min(1499, myStats->HUNGER + 100);
								serverUpdateHunger(player);
								if ( stats[player]->type == VAMPIRE )
								{
									steamStatisticUpdateClient(player, STEAM_STAT_BAD_BLOOD, STEAM_STAT_INT, lifeStealAmount);
								}
							}
							if ( playerhit >= 0 )
							{
								Uint32 color = makeColorRGB(255, 0, 0);
								messagePlayerColor(playerhit, MESSAGE_STATUS, color, Language::get(2441));
							}
							else
							{
								Uint32 color = makeColorRGB(0, 255, 0);
								messagePlayerMonsterEvent(player, color, *hitstats, Language::get(2440), Language::get(2439), MSG_COMBAT);
							}
						}
					}

					if ( myStats->helmet && myStats->helmet->type == HAT_CHEF )
					{
						if ( hitstats->HP <= 0 && hit.entity->behavior == &actMonster
							&& gibtype[hitstats->type] == 1 )
						{
							int chance = 20;
							bool cursedChef = false;
							if ( myStats->helmet->beatitude >= 0 || shouldInvertEquipmentBeatitude(myStats) )
							{
								chance -= 5 * abs(myStats->helmet->beatitude);
								chance = std::max(10, chance);
							}
							else
							{
								chance -= 5 * abs(myStats->helmet->beatitude);
								chance = std::max(10, chance);
								cursedChef = true;
							}
							if ( local_rng.rand() % chance == 0 )
							{
								Item* meat = newItem(FOOD_MEAT, (Status)(DECREPIT + local_rng.rand() % 4), 
									0, 1, gibtype[hitstats->type], false, &hitstats->inventory);
								if ( cursedChef )
								{
									meat->status = DECREPIT;
									meat->beatitude = -(local_rng.rand() % 3);
								}
							}
						}
					}

					// vampire blood drops.
					bool tryBloodVial = false;
					if ( hitstats->HP <= 0 && hit.entity->behavior == &actMonster 
						&& (gibtype[hitstats->type] == 1 || gibtype[hitstats->type] == 2) )
					{
						for ( c = 0; c < MAXPLAYERS; ++c )
						{
							if ( playerRequiresBloodToSustain(c) )
							{
								tryBloodVial = true;
								break;
							}
						}
					}
					if ( tryBloodVial )
					{
						bool spawnBloodVial = false;
						bool spawnSecondVial = false;
						if ( (backstab || flanking) && hitstats->HP <= 0 )
						{
							spawnBloodVial = true;
						}
						else if ( hitstats->EFFECTS[EFF_BLEEDING] || myStats->EFFECTS[EFF_VAMPIRICAURA] )
						{
							if ( hitstats->EFFECTS_TIMERS[EFF_BLEEDING] >= 250 )
							{
								spawnBloodVial = (local_rng.rand() % 2 == 0);
							}
							else if ( hitstats->EFFECTS_TIMERS[EFF_BLEEDING] >= 150 )
							{
								spawnBloodVial = (local_rng.rand() % 4 == 0);
							}
							else
							{
								spawnBloodVial = (local_rng.rand() % 8 == 0);
							}

							if ( local_rng.rand() % 5 == 0 )
							{
								spawnSecondVial = true;
							}
						}
						else
						{
							spawnBloodVial = (local_rng.rand() % 10 == 0);
						}

						if ( spawnBloodVial )
						{
							Item* blood = newItem(FOOD_BLOOD, EXCELLENT, 0, 1, gibtype[hitstats->type] - 1, true, &hitstats->inventory);
							if ( spawnSecondVial )
							{
								blood = newItem(FOOD_BLOOD, EXCELLENT, 0, 1, gibtype[hitstats->type] - 1, true, &hitstats->inventory);
							}
						}
					}

					if ( hitstats )
					{
						if ( hitstats->type == BUGBEAR
							&& hitstats->defending )
						{
#ifdef USE_FMOD

							bool playing = false;
							if ( hitstats->monster_sound )
							{
								hitstats->monster_sound->isPlaying(&playing);
							}
							if ( !playing )
							{
								if ( (hitstats->OLDHP == hitstats->HP && local_rng.rand() % 5 == 0)
									|| (local_rng.rand() % 10 == 0) )
								{
									hitstats->monster_sound = playSoundEntity(hit.entity, 681 + local_rng.rand() % 2, 128);
								}
							}
#endif
						}
					}
				}
			}
		}
		else
		{
			if ( !miss && ((dist != STRIKERANGE && !whip) || (dist != STRIKERANGE * 1.5 && whip)) )
			{
				// hit a wall
				if ( pose == PLAYER_POSE_GOLEM_SMASH )
				{
					if ( hit.mapx >= 1 && hit.mapx < map.width - 1 && hit.mapy >= 1 && hit.mapy < map.height - 1 )
					{
						if ( magicDig(this, nullptr, 0, 0) )
						{
							playSoundPos(hit.x, hit.y, 67, 128); // bust wall
							if ( player >= 0 && myStats->type == TROLL )
							{
								serverUpdatePlayerGameplayStats(player, STATISTICS_FORUM_TROLL, AchievementObserver::FORUM_TROLL_BREAK_WALL);
							}
							for ( int c = 0; c < 5; c++ )
							{
								Entity* entity = newEntity(78, 1, map.entities, nullptr); //Particle entity.
								entity->sizex = 1;
								entity->sizey = 1;
								entity->x = hit.x + (-4 + local_rng.rand() % 9);
								entity->y = hit.y + (-4 + local_rng.rand() % 9);
								entity->z = 7.5;
								entity->yaw = c * 2 * PI / 5;//(local_rng.rand() % 360) * PI / 180.0;
								entity->roll = (local_rng.rand() % 360) * PI / 180.0;

								entity->vel_x = 0.2 * cos(entity->yaw);
								entity->vel_y = 0.2 * sin(entity->yaw);
								entity->vel_z = 3;// 0.25 - (local_rng.rand() % 5) / 10.0;

								entity->skill[0] = 50; // particle life
								entity->skill[1] = 0; // particle direction, 0 = upwards, 1 = downwards.

								entity->behavior = &actParticleRock;
								entity->flags[PASSABLE] = true;
								entity->flags[NOUPDATE] = true;
								entity->flags[UNCLICKABLE] = true;
								if ( multiplayer != CLIENT )
								{
									entity_uids--;
								}
								entity->setUID(-3);
							}
						}
					}
					else
					{
						messagePlayer(player, MESSAGE_HINT, Language::get(706));
					}
				}
				else if ( myStats->weapon != NULL && !shapeshifted )
				{
					if ( myStats->weapon->type == TOOL_PICKAXE )
					{
						if ( hit.mapx >= 1 && hit.mapx < map.width - 1 && hit.mapy >= 1 && hit.mapy < map.height - 1 )
						{
							bool degradePickaxe = true;
							if ( MFLAG_DISABLEDIGGING )
							{
								Uint32 color = makeColorRGB(255, 0, 255);
								if ( this->behavior == &actPlayer )
								{
									messagePlayerColor(this->skill[2], MESSAGE_HINT, color, Language::get(2380)); // disabled digging.
								}
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
							else if ( !mapTileDiggable(hit.mapx, hit.mapy) )
							{
								spawnBang(hit.x - cos(yaw) * 2, hit.y - sin(yaw) * 2, 0);
								messagePlayer(player, MESSAGE_HINT, Language::get(706));
							}
							else
							{
								playSoundPos(hit.x, hit.y, 67, 128); // bust wall
								// spawn several rock items
								i = 8 + local_rng.rand() % 4;
								for ( c = 0; c < i; c++ )
								{
									Entity* entity = newEntity(-1, 1, map.entities, nullptr); //Rock/item entity.
									entity->flags[INVISIBLE] = true;
									entity->flags[UPDATENEEDED] = true;
									entity->x = hit.mapx * 16 + 4 + local_rng.rand() % 8;
									entity->y = hit.mapy * 16 + 4 + local_rng.rand() % 8;
									entity->z = -6 + local_rng.rand() % 12;
									entity->sizex = 4;
									entity->sizey = 4;
									entity->yaw = local_rng.rand() % 360 * PI / 180;
									entity->vel_x = (local_rng.rand() % 20 - 10) / 10.0;
									entity->vel_y = (local_rng.rand() % 20 - 10) / 10.0;
									entity->vel_z = -.25 - (local_rng.rand() % 5) / 10.0;
									entity->flags[PASSABLE] = true;
									entity->behavior = &actItem;
									entity->flags[USERFLAG1] = true; // no collision: helps performance
									entity->skill[10] = GEM_ROCK;    // type
									entity->skill[11] = WORN;        // status
									entity->skill[12] = 0;           // beatitude
									entity->skill[13] = 1;           // count
									entity->skill[14] = 0;           // appearance
									entity->skill[15] = 1;			 // identified
								}

								if ( map.tiles[OBSTACLELAYER + hit.mapy * MAPLAYERS + hit.mapx * MAPLAYERS * map.height] >= 41
									&& map.tiles[OBSTACLELAYER + hit.mapy * MAPLAYERS + hit.mapx * MAPLAYERS * map.height] <= 49 )
								{
									steamAchievementClient(player, "BARONY_ACH_BAD_REVIEW");
								}

								if ( behavior == &actPlayer )
								{
									if ( myStats->weapon && myStats->weapon->type == TOOL_PICKAXE )
									{
										Compendium_t::Events_t::eventUpdate(skill[2], Compendium_t::CPDM_PICKAXE_WALLS_DUG, myStats->weapon->type, 1);
									}
								}

								map.tiles[OBSTACLELAYER + hit.mapy * MAPLAYERS + hit.mapx * MAPLAYERS * map.height] = 0;
								// send wall destroy info to clients
								if ( multiplayer == SERVER )
								{
									for ( c = 1; c < MAXPLAYERS; c++ )
									{
										if ( players[c]->isLocalPlayer() || client_disconnected[c] == true )
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
							int chance = 2 + (myStats->type == GOBLIN ? 2 : 0);
							if ( local_rng.rand() % chance && degradePickaxe && myStats->weapon )
							{
								if ( myStats->weapon->status > BROKEN )
								{
									myStats->weapon->status = static_cast<Status>(myStats->weapon->status - 1);
								}
								if ( myStats->weapon->status == BROKEN )
								{
									messagePlayer(player, MESSAGE_EQUIPMENT, Language::get(704));
									playSoundEntity(this, 76, 64);

									if ( behavior == &actPlayer )
									{
										if ( myStats->weapon && myStats->weapon->type == TOOL_PICKAXE )
										{
											Compendium_t::Events_t::eventUpdate(skill[2], Compendium_t::CPDM_BROKEN, myStats->weapon->type, 1);
										}
									}
								}
								else
								{
									messagePlayer(player, MESSAGE_EQUIPMENT, Language::get(705));
									if ( behavior == &actPlayer && myStats->weapon )
									{
										Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_DEGRADED, myStats->weapon->type, 1);
									}
								}
								if ( player > 0 && multiplayer == SERVER && !players[player]->isLocalPlayer() )
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
							messagePlayer(player, MESSAGE_HINT, Language::get(706));
						}
					}
					else
					{
						// bang
						spawnBang(hit.x - cos(yaw) * 2, hit.y - sin(yaw) * 2, 0);

						if ( behavior == &actPlayer )
						{
							Compendium_t::Events_t::eventUpdate(skill[2], Compendium_t::CPDM_ATTACKS_MISSES,
								myStats->weapon->type, 1);
						}
					}
				}
				else
				{
					// bang
					//spawnBang(hit.x - cos(my->yaw)*2,hit.y - sin(my->yaw)*2,0);
					playSoundPos(hit.x, hit.y, 183, 64);

					if ( !myStats->weapon && !shapeshifted )
					{
						if ( myStats->gloves )
						{
							switch ( myStats->gloves->type )
							{
							case BRASS_KNUCKLES:
							case IRON_KNUCKLES:
							case SPIKED_GAUNTLETS:
								if ( behavior == &actPlayer )
								{
									Compendium_t::Events_t::eventUpdate(skill[2], Compendium_t::CPDM_ATTACKS_MISSES,
										myStats->gloves->type, 1);
								}
								break;
							default:
								break;
							}
						}
					}
				}
			}
			else
			{
				// hit nothing
				if ( myStats->weapon != NULL && !shapeshifted )
				{
					if ( behavior == &actPlayer )
					{
						Compendium_t::Events_t::eventUpdate(skill[2], Compendium_t::CPDM_ATTACKS_MISSES,
							myStats->weapon->type, 1);
					}
				}
				else if ( !myStats->weapon && !shapeshifted )
				{
					if ( myStats->gloves )
					{
						switch ( myStats->gloves->type )
						{
						case BRASS_KNUCKLES:
						case IRON_KNUCKLES:
						case SPIKED_GAUNTLETS:
							if ( behavior == &actPlayer )
							{
								Compendium_t::Events_t::eventUpdate(skill[2], Compendium_t::CPDM_ATTACKS_MISSES,
									myStats->gloves->type, 1);
							}
							break;
						default:
							break;
						}
					}
				}
			}

			// apply AoE shake effect
			if ( (pose == MONSTER_POSE_GOLEM_SMASH || pose == PLAYER_POSE_GOLEM_SMASH) && target == nullptr )
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
						if ( playerhit > 0 && multiplayer == SERVER && !players[playerhit]->isLocalPlayer() )
						{
							strcpy((char*)net_packet->data, "SHAK");
							net_packet->data[4] = 10; // turns into .1
							net_packet->data[5] = 10;
							net_packet->address.host = net_clients[playerhit - 1].host;
							net_packet->address.port = net_clients[playerhit - 1].port;
							net_packet->len = 6;
							sendPacketSafe(net_sock, -1, net_packet, playerhit - 1);
						}
						else if ( playerhit >= 0 && players[playerhit]->isLocalPlayer() )
						{
							cameravars[playerhit].shakex += .1;
							cameravars[playerhit].shakey += 10;
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

	Entity* playerEntity = nullptr;
	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		if ( stat && stats[i] == stat )
		{
			if ( players[i] && players[i]->entity )
			{
				playerEntity = players[i]->entity;
				break;
			}
		}
	}
	int armor = statGetCON(stat, playerEntity);

	if ( stat->helmet )
	{
		armor += stat->helmet->armorGetAC(stat);
	}
	if ( stat->breastplate )
	{
		armor += stat->breastplate->armorGetAC(stat);
	}
	if ( stat->gloves )
	{
		armor += stat->gloves->armorGetAC(stat);
	}
	if ( stat->shoes )
	{
		armor += stat->shoes->armorGetAC(stat);
	}
	if ( stat->shield )
	{
		armor += stat->shield->armorGetAC(stat);
	}
	if ( stat->cloak )
	{
		armor += stat->cloak->armorGetAC(stat);
	}
	if ( stat->ring )
	{
		armor += stat->ring->armorGetAC(stat);
	}
	if ( stat->mask )
	{
		armor += stat->mask->armorGetAC(stat);
	}

	if ( stat->type == TROLL || stat->type == RAT || stat->type == SPIDER || stat->type == CREATURE_IMP )
	{
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			if ( stat == stats[i] ) // is a player stat pointer.
			{
				return armor; // shapeshifted players do not benefit from shield defense/proficiency.
			}
		}
	}

	if ( stat->shield )
	{
		int shieldskill = stat->getPassiveShieldBonus(true, false);
		armor += shieldskill;
		if ( stat->defending )
		{
			//messagePlayer(0, "shield up! +%d", 5 + stat->PROFICIENCIES[PRO_SHIELD] / 5);
			armor += stat->getActiveShieldBonus(true, false);
		}
	}
	if ( stat->type == MIMIC && stat->EFFECTS[EFF_MIMIC_LOCKED] )
	{
		armor *= 2;
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
			Uint32 color = makeColorRGB(255, 0, 255);
			// play sound effect
			playSoundEntity(this, 77, 64);
			messagePlayerColor(player, MESSAGE_HINT, color, Language::get(2381));
			return false;
		}
	}
	else if ( behavior == &actDeathGhost )
	{
		player = skill[2];
	}

	if ( (strstr(map.name, "Minotaur") && behavior != &actDeathGhost) 
		|| checkObstacle((tele_x << 4) + 8, (tele_y << 4) + 8, this, NULL) )
	{
		messagePlayer(player, MESSAGE_HINT, Language::get(707));
		return false;
	}

	// play sound effect
	int sfx = 77;
	if ( behavior == &actDeathGhost )
	{
		sfx = 608 + local_rng.rand() % 3;
		playSoundEntity(this, sfx, 128);
	}
	else
	{
		playSoundEntity(this, sfx, 64);
	}
    spawnPoof(x, y, 0, 1.0, true);

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
			messagePlayer(player, MESSAGE_HINT, Language::get(707));
		}
		return false;
	}
	updateAchievementBaitAndSwitch(player, true);
	if ( multiplayer != CLIENT )
	{
		TileEntityList.updateEntity(*this);
	}
	if ( multiplayer == SERVER && player > 0 && !players[player]->isLocalPlayer() )
	{
		strcpy((char*)net_packet->data, "TELE");
		net_packet->data[4] = tele_x;
		net_packet->data[5] = tele_y;
		SDLNet_Write16(static_cast<Sint16>(this->yaw * 180 / PI), &net_packet->data[6]);
		net_packet->address.host = net_clients[player - 1].host;
		net_packet->address.port = net_clients[player - 1].port;
		net_packet->len = 8;
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
	if ( behavior == &actDeathGhost )
	{
		playSoundEntity(this, sfx, 128);
	}
	else
	{
		playSoundEntity(this, sfx, 64);
	}
    const float poofx = x + cosf(yaw) * 4.f;
    const float poofy = y + sinf(yaw) * 4.f;
    spawnPoof(poofx, poofy, 0, 1.0, true);
    bNeedsRenderPositionInit = true;
    for (auto part : bodyparts) {
        part->bNeedsRenderPositionInit = true;
    }
    for (auto node = map.entities->first; node != nullptr; node = node->next) {
        auto entity = (Entity*)node->element;
        if (entity && entity->behavior == &actSpriteNametag) {
            if (entity->parent == uid) {
                entity->bNeedsRenderPositionInit = true;
            }
        }
    }
    if (player == clientnum || (splitscreen && player >= 0)) {
        temporarilyDisableDithering();
    }

	if ( behavior == &actMonster )
	{
		achievementObserver.addEntityAchievementTimer(this, AchievementObserver::BARONY_ACH_TELEFRAG, 50, true, 0);
		achievementObserver.addEntityAchievementTimer(this, AchievementObserver::BARONY_ACH_COWBOY_FROM_HELL, 150, true, 0);
	}

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
			Uint32 color = makeColorRGB(255, 0, 255);
			// play sound effect
			playSoundEntity(this, 77, 64);
			messagePlayerColor(player, MESSAGE_HINT, color, Language::get(2381));
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
		messagePlayer(player, MESSAGE_HINT, Language::get(708));
		return false;
	}
	pickedlocation = local_rng.rand() % numlocations;
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

bool teleportCoordHasTrap(const int x, const int y)
{
	std::vector<list_t*> entLists = TileEntityList.getEntitiesWithinRadius(x, y, 0);
	for ( auto it = entLists.begin(); it != entLists.end(); ++it )
	{
		list_t* currentList = *it;
		node_t* node;
		for ( node = currentList->first; node != nullptr; node = node->next )
		{
			Entity* entity = (Entity*)node->element;
			if ( !entity ) { continue; }
			if ( entity->behavior == &actSpearTrap )
			{
				int i = static_cast<int>(entity->x) >> 4;
				int j = static_cast<int>(entity->y) >> 4;
				if ( i == x && j == y )
				{
					return true;
				}
			}
		}
	}
	return false;
}

bool Entity::teleportAroundEntity(Entity* target, int dist, int effectType)
{
	int numlocations = 0;
	int pickedlocation;
	int player = -1;
	if ( !target )
	{
		return false;
	}
	int ty = static_cast<int>(std::floor(target->y)) >> 4;
	int tx = static_cast<int>(std::floor(target->x)) >> 4;
	if ( target->behavior == &::actTeleportShrine )
	{
		tx = static_cast<int>(std::floor(target->x + 32.0 * cos(target->yaw))) >> 4;
		ty = static_cast<int>(std::floor(target->y + 32.0 * sin(target->yaw))) >> 4;

		if ( target->shrineDestXOffset != 0 || target->shrineDestYOffset != 0 )
		{
			// default both to ontop of the shrine
			tx = static_cast<int>(std::floor(target->x)) >> 4;
			ty = static_cast<int>(std::floor(target->y)) >> 4;
		}
		if ( target->shrineDestXOffset != 0 )
		{
			tx = (static_cast<int>(std::floor(target->x)) >> 4) + target->shrineDestXOffset;
		}
		if ( target->shrineDestYOffset != 0 )
		{
			ty = (static_cast<int>(std::floor(target->y)) >> 4) + target->shrineDestYOffset;
		}
	}

	if ( behavior == &actPlayer )
	{
		player = skill[2];
		if ( MFLAG_DISABLETELEPORT )
		{
			Uint32 color = makeColorRGB(255, 0, 255);
			// play sound effect
			playSoundEntity(this, 77, 64);
			messagePlayerColor(player, MESSAGE_HINT, color, Language::get(2381));
			return false;
		}
	}
	else if ( behavior == &actDeathGhost )
	{
		player = skill[2];
	}

	struct Coord_t
	{
		bool onHazard = false;
		int x = 0;
		int y = 0;
		Coord_t(const int _x, const int _y, const bool _onHazard)
		{
			x = _x;
			y = _y;
			onHazard = _onHazard;
		};
	};
	std::vector<Coord_t> goodspots;
	std::vector<Coord_t> spotsBehindMonster;
	bool forceSpot = false;
	for ( int iy = std::max(1, ty - dist); !forceSpot && iy <= std::min(ty + dist, static_cast<int>(map.height) - 1); ++iy )
	{
		for ( int ix = std::max(1, tx - dist); !forceSpot && ix <= std::min(tx + dist, static_cast<int>(map.width) - 1); ++ix )
		{
			if ( !checkObstacle((ix << 4) + 8, (iy << 4) + 8, this, NULL) )
			{
				if ( behavior == &actPlayer && target->behavior == &actMonster )
				{
					// check LOS
					Entity* ohit = hit.entity;

					// pretend player has teleported, get the angle needed.
					real_t tmpx = x;
					real_t tmpy = y;
					x = (ix << 4) + 8;
					y = (iy << 4) + 8;
					TileEntityList.updateEntity(*this); // important - lineTrace needs the TileEntityListUpdated.

					real_t tangent = atan2(target->y - this->y, target->x - this->x);
					lineTraceTarget(this, this->x, this->y, tangent, 64 * dist, 0, false, target);
					if ( hit.entity == target && !entityInsideSomething(this) )
					{
						numlocations++;
						real_t targetYaw = target->yaw;
						while ( targetYaw >= 2 * PI )
						{
							targetYaw -= PI * 2;
						}
						while ( targetYaw < 0 )
						{
							targetYaw += PI * 2;
						}
						real_t yawDifference = (PI - abs(abs(tangent - targetYaw) - PI)) * 2;
						if ( yawDifference >= 0 && yawDifference <= PI ) // 180 degree arc
						{
							spotsBehindMonster.push_back(Coord_t(ix, iy, teleportCoordHasTrap(ix, iy)));
						}
						else
						{
							goodspots.push_back(Coord_t(ix, iy, teleportCoordHasTrap(ix, iy)));
						}
					}
					// restore coordinates.
					x = tmpx;
					y = tmpy;
					TileEntityList.updateEntity(*this); // important - lineTrace needs the TileEntityListUpdated.
					hit.entity = ohit;
				}
				else
				{
					if ( target->behavior == &actBomb && target->skill[22] == 1 && ix == tx && iy == ty ) // teleport receiver.
					{
						// directly on top, let's go there.
						real_t tmpx = x;
						real_t tmpy = y;
						x = (ix << 4) + 8;
						y = (iy << 4) + 8;
						if ( !entityInsideSomething(this) )
						{
							bool onTrap = false;
							if ( !onTrap )
							{
								forceSpot = true;
								goodspots.clear();
								goodspots.push_back(Coord_t(ix, iy, onTrap));
								numlocations = 1;
								// restore coordinates.
								x = tmpx;
								y = tmpy;
								break;
							}
							else
							{
								goodspots.push_back(Coord_t(ix, iy, onTrap));
								numlocations++;
							}
						}
						// restore coordinates.
						x = tmpx;
						y = tmpy;
					}
					else if ( target->behavior == &::actTeleportShrine && ix == tx && iy == ty )
					{
						// directly on top, let's go there.
						real_t tmpx = x;
						real_t tmpy = y;
						x = (ix << 4) + 8;
						y = (iy << 4) + 8;
						if ( !entityInsideSomething(this) )
						{
							bool onTrap = true;
							if ( behavior == &actDeathGhost )
							{
								onTrap = false;
							}
							else
							{
								onTrap = teleportCoordHasTrap(ix, iy);
							}
							if ( !onTrap )
							{
								forceSpot = true;
								goodspots.clear();
								goodspots.push_back(Coord_t(ix, iy, onTrap));
								numlocations = 1;
								// restore coordinates.
								x = tmpx;
								y = tmpy;
								break;
							}
							else
							{
								goodspots.push_back(Coord_t(ix, iy, onTrap));
								numlocations++;
							}
						}
						// restore coordinates.
						x = tmpx;
						y = tmpy;
					}
					else
					{
						real_t tmpx = x;
						real_t tmpy = y;
						x = (ix << 4) + 8;
						y = (iy << 4) + 8;
						if ( !entityInsideSomething(this) )
						{
							if ( behavior == &actDeathGhost )
							{
								goodspots.push_back(Coord_t(ix, iy, false));
							}
							else
							{
								goodspots.push_back(Coord_t(ix, iy, teleportCoordHasTrap(ix, iy)));
							}
							numlocations++;
						}
						// restore coordinates.
						x = tmpx;
						y = tmpy;
					}
				}
			}
		}
	}
	//messagePlayer(0, "locations: %d", numlocations);
	if ( numlocations == 0 )
	{
		messagePlayer(player, MESSAGE_HINT, Language::get(708));
		return false;
	}
	if ( behavior == &actMonster || spotsBehindMonster.empty() )
	{
		std::vector<unsigned int> goodchances;
		std::vector<unsigned int> badchances;
		bool foundGoodSpot = false;
		bool foundBadSpot = false;
		for ( auto& coord : goodspots )
		{
			if ( coord.onHazard )
			{
				foundBadSpot = true;
				badchances.push_back(1);
				goodchances.push_back(0);
			}
			else
			{
				foundGoodSpot = true;
				badchances.push_back(0);
				goodchances.push_back(1);
			}
		}

		if ( foundGoodSpot )
		{
			auto& coord = goodspots[local_rng.discrete(goodchances.data(), goodchances.size())];
			tx = coord.x;
			ty = coord.y;
		}
		else
		{
			auto& coord = goodspots[local_rng.discrete(badchances.data(), badchances.size())];
			tx = coord.x;
			ty = coord.y;
		}
	}
	else
	{
		std::vector<unsigned int> goodchances;
		std::vector<unsigned int> badchances;
		bool foundGoodSpot = false;
		bool foundBadSpot = false;
		for ( auto& coord : spotsBehindMonster )
		{
			if ( coord.onHazard )
			{
				foundBadSpot = true;
				badchances.push_back(1);
				goodchances.push_back(0);
			}
			else
			{
				foundGoodSpot = true;
				badchances.push_back(0);
				goodchances.push_back(1);
			}
		}

		if ( foundGoodSpot )
		{
			auto& coord = spotsBehindMonster[local_rng.discrete(goodchances.data(), goodchances.size())];
			tx = coord.x;
			ty = coord.y;
		}
		else
		{
			auto& coord = spotsBehindMonster[local_rng.discrete(badchances.data(), badchances.size())];
			tx = coord.x;
			ty = coord.y;
		}
	}
	if ( behavior == &actPlayer || behavior == &actDeathGhost )
	{
		// pretend player has teleported, get the angle needed.
		real_t tmpx = x;
		real_t tmpy = y;
		x = (tx << 4) + 8;
		y = (ty << 4) + 8;
		real_t tangent = atan2(target->y - this->y, target->x - this->x);
		// restore coordinates.
		x = tmpx;
		y = tmpy;
		this->yaw = tangent;
		if ( target->behavior == &actMonster && target->monsterTarget == getUID() )
		{
			target->monsterReleaseAttackTarget();
		}
		if ( teleport(tx, ty) )
		{
			return true;
		}
		return false;
	}

	return teleport(tx, ty);
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
	//	messagePlayer(player, Language::get(707));
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
			messagePlayer(player, MESSAGE_HINT, Language::get(707));
		}
		return false;
	}*/
	if ( multiplayer != CLIENT )
	{
		TileEntityList.updateEntity(*this);
	}
	if ( player > 0 && multiplayer == SERVER && !players[player]->isLocalPlayer() )
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

	if ( src->behavior == &actPlayer && behavior == &actMonster && root )
	{
		Compendium_t::Events_t::eventUpdateMonster(src->skill[2], Compendium_t::CPDM_KILLED_BY, this, 1);
	}

	if ( src->behavior == &actMonster 
		&& (src->monsterAllySummonRank != 0
			|| src->monsterIsTinkeringCreation()) )
	{
		if ( root )
		{
			if ( src->monsterIsTinkeringCreation() )
			{
				int compendiumPlayer = behavior == &actPlayer ? skill[2] : -1;
				if ( behavior == &actMonster )
				{
					if ( auto leader = monsterAllyGetPlayerLeader() )
					{
						compendiumPlayer = leader->skill[2];
					}
				}
				if ( multiplayer == SINGLE )
				{
					if ( splitscreen )
					{
						Compendium_t::Events_t::eventUpdateMonster(compendiumPlayer, Compendium_t::CPDM_KILLED_MULTIPLAYER, src, 1);
					}
					else
					{
						Compendium_t::Events_t::eventUpdateMonster(compendiumPlayer, Compendium_t::CPDM_KILLED_SOLO, src, 1);
					}
				}
				else
				{
					Compendium_t::Events_t::eventUpdateMonster(compendiumPlayer, Compendium_t::CPDM_KILLED_MULTIPLAYER, src, 1);
					for ( int i = 0; i < MAXPLAYERS; ++i )
					{
						if ( !client_disconnected[i] )
						{
							Compendium_t::Events_t::eventUpdateMonster(i, Compendium_t::CPDM_KILLED_PARTY, src, 1);
						}
					}
				}
			}
		}
		return; // summoned monster, no XP!
	}
	if ( srcStats->type == INCUBUS && !strncmp(srcStats->name, "inner demon", strlen("inner demon")) )
	{
		return;
	}

	int player = -1;
	if ( behavior == &actPlayer )
	{
		player = skill[2];
		if ( src->behavior == &actPlayer && root )
		{
			return;
		}
	}

	// calculate XP gain
	int baseXp = 10;
	if ( srcStats->type == BAT_SMALL )
	{
		baseXp = 1 + local_rng.rand() % 2;
	}
	int xpGain = baseXp + local_rng.rand() % std::max(1, baseXp) + std::max(0, srcStats->LVL - destStats->LVL) * baseXp;
	if ( srcStats->MISC_FLAGS[STAT_FLAG_XP_PERCENT_AWARD] > 0 )
	{
		int value = srcStats->MISC_FLAGS[STAT_FLAG_XP_PERCENT_AWARD] - 1; // offset by 1 since 0 is nothing
		double percent = value / 100.f;
		xpGain = percent * xpGain;
	}
	if ( gameplayCustomManager.inUse() )
	{
		xpGain = (gameplayCustomManager.globalXPPercent / 100.f) * xpGain;
	}
	else if ( gameModeManager.currentSession.challengeRun.isActive()
		&& gameModeManager.currentSession.challengeRun.globalXPPercent != 100 )
	{
		xpGain *= gameModeManager.currentSession.challengeRun.globalXPPercent / 100.0;
	}

	// save hit struct
	hit_t tempHit;
	tempHit.entity = hit.entity;
	tempHit.mapx = hit.mapx;
	tempHit.mapy = hit.mapy;
	tempHit.side = hit.side;
	tempHit.x = hit.x;
	tempHit.y = hit.y;

	int shareRange = gameplayCustomManager.inUse() ? gameplayCustomManager.xpShareRange : XPSHARERANGE;

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
				if ( entityDist(this, entity) < shareRange )
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
						Entity* follower = nullptr;
						if ( (Uint32*)node->element )
						{
							follower = uidToEntity(*((Uint32*)node->element));
						}
						if ( follower && entityDist(this, follower) < shareRange && follower != src )
						{
							if ( follower->monsterIsTinkeringCreation() )
							{
								--numFollowers; // tinkering creation don't penalise XP.
								continue;
							}
							Stat* followerStats = follower->getStats();
							if ( followerStats )
							{
								int inspiration = follower->getEntityInspirationFromAllies();
								real_t inspirationMult = (1.0 + (inspiration / 100.0));

								//int xpDivide = std::min(std::max(1, numFollowers), 4); // 1 - 4 depending on followers.
								if ( follower->monsterAllySummonRank != 0 && numshares > 0 )
								{
									int gain = (xpGain * numshares); // summoned monsters aren't penalised XP.
									if ( inspiration )
									{
										int oldGain = gain;
										gain *= inspirationMult;
										if ( ((followerStats->EXP + gain) >= 100) && ((followerStats->EXP + (xpGain * numshares)) < 100) )
										{
											// inspiration caused us to level
											steamAchievementEntity(this, "BARONY_ACH_BY_EXAMPLE");
										}

										if ( gain > oldGain )
										{
											Compendium_t::Events_t::eventUpdate(this->skill[2], Compendium_t::CPDM_INSPIRATION_XP, HAT_CROWN, gain - oldGain);
										}
									}
									followerStats->EXP += gain; 
								}
								else
								{
									int gain = xpGain;
									if ( inspiration )
									{
										int oldGain = gain;
										gain *= inspirationMult;
										if ( ((followerStats->EXP + gain) >= 100) && ((followerStats->EXP + xpGain) < 100) )
										{
											// inspiration caused us to level
											steamAchievementEntity(this, "BARONY_ACH_BY_EXAMPLE");
										}

										if ( gain > oldGain )
										{
											Compendium_t::Events_t::eventUpdate(this->skill[2], Compendium_t::CPDM_INSPIRATION_XP, HAT_CROWN, gain - oldGain);
										}
									}
									followerStats->EXP += gain;
								}
								//messagePlayer(0, "monster got %d xp", xpGain);
							}
						}
					}
				}
			}
		}
		
	}

	// award XP to main victor
	if ( !this->monsterIsTinkeringCreation() )
	{
		int inspiration = getEntityInspirationFromAllies();
		real_t inspirationMult = (1.0 + (inspiration / 100.0));
		int gain = xpGain;
		if ( inspiration )
		{
			int oldGain = gain;
			gain *= inspirationMult;
			if ( ((destStats->EXP + gain) >= 100) && ((destStats->EXP + xpGain) < 100) )
			{
				// inspiration caused us to level
				if ( behavior == &actMonster )
				{
					if ( auto leader = monsterAllyGetPlayerLeader() )
					{
						steamAchievementEntity(leader, "BARONY_ACH_BY_EXAMPLE");
					}
				}
			}

			if ( gain > oldGain )
			{
				if ( behavior == &actMonster )
				{
					if ( auto leader = monsterAllyGetPlayerLeader() )
					{
						Compendium_t::Events_t::eventUpdate(leader->skill[2], Compendium_t::CPDM_INSPIRATION_XP, HAT_CROWN, gain - oldGain);
					}
				}
			}
		}

		if ( behavior == &actPlayer )
		{
			Compendium_t::Events_t::eventUpdateCodex(skill[2], Compendium_t::CPDM_XP_MAX_IN_FLOOR, "xp", gain, false, -1, true);
			Compendium_t::Events_t::eventUpdateCodex(skill[2], Compendium_t::CPDM_XP_MAX_INSTANCE, "xp", gain);
			Compendium_t::Events_t::eventUpdateCodex(skill[2], Compendium_t::CPDM_XP_KILLS, "xp", gain);

			Compendium_t::Events_t::eventUpdateMonster(skill[2], Compendium_t::CPDM_KILL_XP, src, gain);
		}
		destStats->EXP += gain;
	}

	if ( (srcStats->type == LICH || srcStats->type == LICH_FIRE || srcStats->type == LICH_ICE) && root
		&& this->behavior == &actMonster )
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

	if ( root ) // global stats
	{
		if ( src->behavior == &actPlayer && this->behavior == &actMonster )
		{
			achievementObserver.updateGlobalStat(getIndexForDeathType(destStats->type));
			if ( destStats->type == MIMIC )
			{
				steamAchievementClient(src->skill[2], "BARONY_ACH_ETERNAL_REWARD");
			}
		}
		else if ( src->behavior == &actMonster && this->behavior == &actPlayer )
		{
			if ( srcStats->type == LICH )
			{
				achievementObserver.updateGlobalStat(STEAM_GSTAT_HERX_SLAIN);
			}
			else if ( srcStats->type == LICH_FIRE )
			{
				achievementObserver.updateGlobalStat(STEAM_GSTAT_TWINSFIRE_SLAIN);
			}
			else if ( srcStats->type == LICH_ICE )
			{
				achievementObserver.updateGlobalStat(STEAM_GSTAT_TWINSICE_SLAIN);
			}
			else if ( srcStats->type == DEVIL )
			{
				achievementObserver.updateGlobalStat(STEAM_GSTAT_BAPHOMET_SLAIN);
			}
			else if ( srcStats->type == MINOTAUR )
			{
				achievementObserver.updateGlobalStat(STEAM_GSTAT_MINOTAURS_SLAIN);
			}
			else if ( srcStats->type == SHOPKEEPER )
			{
				achievementObserver.updateGlobalStat(STEAM_GSTAT_SHOPKEEPERS_SLAIN);
			}
		}
	}

	bool killIncrementEvent = false;

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
		if ( srcStats->type == LICH || srcStats->type == LICH_FIRE || srcStats->type == LICH_ICE
			|| srcStats->type == DEVIL )
		{
			if ( gameModeManager.currentSession.challengeRun.isActive() )
			{
				if ( gameModeManager.currentSession.challengeRun.classnum >= 0
					|| gameModeManager.currentSession.challengeRun.race >= 0 )
				{
					for ( int c = 0; c < MAXPLAYERS; c++ )
					{
						steamAchievementClient(c, "BARONY_ACH_BLOOM_PLANTED");
					}
				}
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
		if ( root )
		{
			achievementObserver.awardAchievementIfActive(player, src, AchievementObserver::BARONY_ACH_TELEFRAG);
			if ( stats[player]->playerRace == RACE_INCUBUS && stats[player]->stat_appearance == 0 )
			{
				achievementObserver.playerAchievements[player].checkTraditionKill(this, src);
			}
			if ( stats[player]->type == SPIDER && srcStats->EFFECTS[EFF_WEBBED] )
			{
				steamStatisticUpdateClient(player, STEAM_STAT_MANY_PEDI_PALP, STEAM_STAT_INT, 1);
			}

			bool guerillaRadio = false;
			if ( src->monsterTarget != 0 )
			{
				Entity* wasTargeting = uidToEntity(src->monsterTarget);
				if ( wasTargeting )
				{
					if ( src->monsterState == MONSTER_STATE_ATTACK && wasTargeting->getMonsterTypeFromSprite() == DUMMYBOT )
					{
						steamStatisticUpdateClient(player, STEAM_STAT_RAGE_AGAINST, STEAM_STAT_INT, 1);
					}
					else if ( wasTargeting->behavior == &actDecoyBox )
					{
						guerillaRadio = true;
					}
				}
			}
			if ( !guerillaRadio )
			{
				Entity* noisemaker = uidToEntity(src->monsterLastDistractedByNoisemaker);
				if ( noisemaker && noisemaker->behavior == &actDecoyBox
					&& entityDist(noisemaker, src) < TOUCHRANGE )
				{
					guerillaRadio = true;
				}
			}

			if ( guerillaRadio )
			{
				steamStatisticUpdateClient(player, STEAM_STAT_GUERILLA_RADIO, STEAM_STAT_INT, 1);
				if ( local_rng.rand() % 5 == 0 || (uidToEntity(src->monsterTarget) != this && local_rng.rand() % 3 == 0) )
				{
					this->increaseSkill(PRO_LOCKPICKING);
				}
			}
			killIncrementEvent = true;
		}

		if ( root && srcStats->type == SHOPKEEPER )
		{
			ShopkeeperPlayerHostility.onShopkeeperDeath(src, srcStats, players[player]->entity);
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
		else if ( multiplayer == SERVER && player > 0 && !client_disconnected[player] && !players[player]->isLocalPlayer() )
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
			net_packet->data[11] = (Uint8)destStats->EXP;
			net_packet->data[12] = (Uint8)destStats->LVL;
			SDLNet_Write16((Sint16)destStats->HP, &net_packet->data[13]);
			SDLNet_Write16((Sint16)destStats->MAXHP, &net_packet->data[15]);
			SDLNet_Write16((Sint16)destStats->MP, &net_packet->data[17]);
			SDLNet_Write16((Sint16)destStats->MAXMP, &net_packet->data[19]);
			net_packet->address.host = net_clients[player - 1].host;
			net_packet->address.port = net_clients[player - 1].port;
			net_packet->len = 21;
			sendPacketSafe(net_sock, -1, net_packet, player - 1);
		}

		if ( root )
		{
			if ( multiplayer == SINGLE )
			{
				if ( splitscreen )
				{
					Compendium_t::Events_t::eventUpdateMonster(player, Compendium_t::CPDM_KILLED_MULTIPLAYER, src, 1);
				}
				else
				{
					Compendium_t::Events_t::eventUpdateMonster(player, Compendium_t::CPDM_KILLED_SOLO, src, 1);
				}
			}
			else
			{
				Compendium_t::Events_t::eventUpdateMonster(player, Compendium_t::CPDM_KILLED_MULTIPLAYER, src, 1);
				for ( int i = 0; i < MAXPLAYERS; ++i )
				{
					if ( !client_disconnected[i] )
					{
						Compendium_t::Events_t::eventUpdateMonster(i, Compendium_t::CPDM_KILLED_PARTY, src, 1);
					}
				}
			}

			for ( int i = 0; i < MAXPLAYERS; ++i )
			{
				if ( stats[i]->helmet && stats[i]->helmet->type == HAT_BOUNTYHUNTER )
				{
					auto& bounties = achievementObserver.playerAchievements[i].bountyTargets;
					if ( bounties.find(src->getUID()) != bounties.end() )
					{
						if ( player == i )
						{
							int bountyGold = 1;
							if ( stats[i]->helmet->beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[i]) )
							{
								bountyGold = 10 + currentlevel + local_rng.rand() % 50;
							}
							else
							{
								bountyGold = 1 + local_rng.rand() % 10;
							}
							messagePlayerColor(i, MESSAGE_COMBAT | MESSAGE_HINT, makeColorRGB(0, 255, 0), Language::get(6101), bountyGold);

							stats[i]->GOLD += bountyGold;
							playSoundEntity(players[i]->entity, 242 + local_rng.rand() % 4, 64);
							if ( multiplayer == SERVER && i > 0 && !client_disconnected[i] && !players[i]->isLocalPlayer() )
							{
								// send the client info on the gold it picked up
								strcpy((char*)net_packet->data, "GOLD");
								SDLNet_Write32(stats[i]->GOLD, &net_packet->data[4]);
								net_packet->address.host = net_clients[i - 1].host;
								net_packet->address.port = net_clients[i - 1].port;
								net_packet->len = 8;
								sendPacketSafe(net_sock, -1, net_packet, i - 1);
							}
						}
						else
						{
							messagePlayerColor(i, MESSAGE_COMBAT | MESSAGE_HINT, makeColorRGB(255, 0, 0), Language::get(6102));
						}
					}
				}
			}
		}
	}
	else
	{
		Entity* leader = nullptr;

		// NPCs with leaders award equal XP to their master (so NPCs don't steal XP gainz)

		if ( (leader = uidToEntity(destStats->leader_uid)) != NULL )
		{
			if ( this->monsterIsTinkeringCreation() )
			{
				if ( local_rng.rand() % 10 == 0 )
				{
					leader->increaseSkill(PRO_LOCKPICKING);
				}
				if ( root && leader->behavior == &actPlayer && srcStats->type == MINOTAUR )
				{
					steamAchievementClient(leader->skill[2], "BARONY_ACH_TIME_TO_PLAN");
				}

				if ( root )
				{
					if ( destStats && (destStats->type == SENTRYBOT || destStats->type == SPELLBOT) )
					{
						if ( leader->behavior == &actPlayer )
						{
							Compendium_t::Events_t::eventUpdate(leader->skill[2],
								Compendium_t::CPDM_SENTRY_DEPLOY_KILLS, destStats->type == SENTRYBOT ? TOOL_SENTRYBOT : TOOL_SPELLBOT, 1);
						}
					}
				}
			}
			else
			{
				if ( srcStats->type != BAT_SMALL )
				{
					leader->increaseSkill(PRO_LEADERSHIP);
				}
			}
			leader->awardXP(src, true, false);

			if ( leader->behavior == &actPlayer && root )
			{
				if ( destStats->monsterIsCharmed == 1 )
				{
					// charmed follower killed something.
					steamStatisticUpdateClient(leader->skill[2], STEAM_STAT_KILL_COMMAND, STEAM_STAT_INT, 1);
				}
				if ( destStats->type == INSECTOID )
				{
					if ( leader->getStats()->playerRace == RACE_INSECTOID && leader->getStats()->stat_appearance == 0 )
					{
						steamStatisticUpdateClient(leader->skill[2], STEAM_STAT_MONARCH, STEAM_STAT_INT, 1);
					}
				}
				killIncrementEvent = true;

				if ( multiplayer == SINGLE )
				{
					if ( splitscreen )
					{
						Compendium_t::Events_t::eventUpdateMonster(leader->skill[2], Compendium_t::CPDM_KILLED_MULTIPLAYER, src, 1);
					}
					else
					{
						Compendium_t::Events_t::eventUpdateMonster(leader->skill[2], Compendium_t::CPDM_KILLED_SOLO, src, 1);
					}
				}
				else
				{
					Compendium_t::Events_t::eventUpdateMonster(leader->skill[2], Compendium_t::CPDM_KILLED_MULTIPLAYER, src, 1);
					for ( int i = 0; i < MAXPLAYERS; ++i )
					{
						if ( !client_disconnected[i] )
						{
							Compendium_t::Events_t::eventUpdateMonster(i, Compendium_t::CPDM_KILLED_PARTY, src, 1);
						}
					}
				}

				Compendium_t::Events_t::eventUpdateWorld(leader->skill[2], Compendium_t::CPDM_FOLLOWER_KILLS, "masons guild", 1);
			}
		}


		if ( root )
		{
			for ( int i = 0; i < MAXPLAYERS; ++i )
			{
				if ( stats[i]->helmet && stats[i]->helmet->type == HAT_BOUNTYHUNTER )
				{
					auto& bounties = achievementObserver.playerAchievements[i].bountyTargets;
					if ( bounties.find(src->getUID()) != bounties.end() )
					{
						if ( leader && leader->behavior == &actPlayer && leader->skill[2] == i )
						{
							int bountyGold = 1;
							if ( stats[i]->helmet->beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[i]) )
							{
								bountyGold = 10 + currentlevel + local_rng.rand() % 50;
							}
							else
							{
								bountyGold = 1 + local_rng.rand() % 10;
							}
							messagePlayerColor(i, MESSAGE_COMBAT | MESSAGE_HINT, makeColorRGB(0, 255, 0), Language::get(6101), bountyGold);

							stats[i]->GOLD += bountyGold;
							playSoundEntity(players[i]->entity, 242 + local_rng.rand() % 4, 64);
							if ( multiplayer == SERVER && i > 0 && !client_disconnected[i] && !players[i]->isLocalPlayer() )
							{
								// send the client info on the gold it picked up
								strcpy((char*)net_packet->data, "GOLD");
								SDLNet_Write32(stats[i]->GOLD, &net_packet->data[4]);
								net_packet->address.host = net_clients[i - 1].host;
								net_packet->address.port = net_clients[i - 1].port;
								net_packet->len = 8;
								sendPacketSafe(net_sock, -1, net_packet, i - 1);
							}
						}
						else
						{
							messagePlayerColor(i, MESSAGE_COMBAT | MESSAGE_HINT, makeColorRGB(255, 0, 0), Language::get(6102));
						}
					}
				}
			}
		}
	}

	if ( killIncrementEvent )
	{
		gameModeManager.currentSession.challengeRun.updateKillEvent(src);
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
	if ( everybodyfriendly || intro )   // friendly monsters mode
	{
		return false;
	}

	if ( (your->behavior == &actPlayer || your->behavior == &actPlayerLimb) && (behavior == &actPlayer || behavior == &actPlayerLimb) )
	{
		return false;
	}

	if ( behavior == &actPlayer && your->behavior == &actMonster && yourStats->monsterForceAllegiance != Stat::MONSTER_FORCE_ALLEGIANCE_NONE )
	{
		if ( yourStats->monsterForceAllegiance == Stat::MONSTER_FORCE_PLAYER_ALLY || yourStats->monsterForceAllegiance == Stat::MONSTER_FORCE_PLAYER_RECRUITABLE )
		{
			return false;
		}
		else if ( yourStats->monsterForceAllegiance == Stat::MONSTER_FORCE_PLAYER_ENEMY )
		{
			return true;
		}
	}
	else if ( your->behavior == &actPlayer && behavior == &actMonster && myStats->monsterForceAllegiance != Stat::MONSTER_FORCE_ALLEGIANCE_NONE )
	{
		if ( myStats->monsterForceAllegiance == Stat::MONSTER_FORCE_PLAYER_ALLY || myStats->monsterForceAllegiance == Stat::MONSTER_FORCE_PLAYER_RECRUITABLE )
		{
			return false;
		}
		else if ( myStats->monsterForceAllegiance == Stat::MONSTER_FORCE_PLAYER_ENEMY )
		{
			return true;
		}
	}

	if ( myStats->type == GYROBOT )
	{
		return false;
	}

	if ( (myStats->type == SHOPKEEPER && myStats->MISC_FLAGS[STAT_FLAG_MYSTERIOUS_SHOPKEEP] > 0)
		|| (yourStats->type == SHOPKEEPER && yourStats->MISC_FLAGS[STAT_FLAG_MYSTERIOUS_SHOPKEEP] > 0) )
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
	else if ( your->behavior == &actPlayer && myStats->type == CREATURE_IMP 
		&& (!strncmp(map.name, "Boss", 4) || !strncmp(map.name, "Hell Boss", 9)) )
	{
		if ( this->monsterAllyGetPlayerLeader() )
		{
			return false;
		}
		return true;
	}
	else if ( behavior == &actPlayer && yourStats->type == CREATURE_IMP 
		&& (!strncmp(map.name, "Boss", 4) || !strncmp(map.name, "Hell Boss", 9)) )
	{
		if ( your->monsterAllyGetPlayerLeader() )
		{
			return false;
		}
		return true;
	}
	else if ( your->behavior == &actPlayer && myStats->type == VAMPIRE && MonsterData_t::nameMatchesSpecialNPCName(*myStats, "bram kindly") )
	{
		return true;
	}
	else if ( behavior == &actPlayer && yourStats->type == VAMPIRE && MonsterData_t::nameMatchesSpecialNPCName(*yourStats, "bram kindly") )
	{
		return true;
	}
	/*else if ( your->behavior == &actPlayer && myStats->type == MIMIC )
	{
		if ( monsterAllyGetPlayerLeader() )
		{
			return false;
		}
		return true;
	}
	else if ( behavior == &actPlayer && yourStats->type == MIMIC )
	{
		if ( your->monsterAllyGetPlayerLeader() )
		{
			return false;
		}
		return true;
	}*/
	else if ( behavior == &actMonster && myStats->type == INCUBUS && !strncmp(myStats->name, "inner demon", strlen("inner demon")) )
	{
		Entity* parentEntity = uidToEntity(this->parent);
		if ( parentEntity != your )
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else if ( behavior == &actPlayer && yourStats->type == INCUBUS && !strncmp(yourStats->name, "inner demon", strlen("inner demon")) )
	{
		Entity* parentEntity = uidToEntity(your->parent);
		if ( parentEntity != this )
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else if ( behavior == &actMonster && your->behavior == &actMonster && yourStats->type == INCUBUS && !strncmp(yourStats->name, "inner demon", strlen("inner demon")) )
	{
		Entity* illusionTauntingThisEntity = uidToEntity(static_cast<Uint32>(your->monsterIllusionTauntingThisUid));
		if ( illusionTauntingThisEntity == this )
		{
			return true;
		}
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
			if ( myStats->type == SHOPKEEPER && your->behavior == &actPlayer )
			{
				result = ShopkeeperPlayerHostility.isPlayerEnemy(your->skill[2]);
			}
			else if ( yourStats->type == SHOPKEEPER && behavior == &actPlayer )
			{
				result = ShopkeeperPlayerHostility.isPlayerEnemy(this->skill[2]);
			}
			else if ( myStats->type == BAT_SMALL && your->behavior == &actPlayer )
			{
				result = true;
			}
			else if ( yourStats->type == BAT_SMALL && behavior == &actPlayer )
			{
				result = true;
			}
			else if ( behavior == &actPlayer && myStats->type != HUMAN )
			{
				result = swornenemies[HUMAN][yourStats->type];
				if ( (yourStats->type == HUMAN || yourStats->type == SHOPKEEPER) && myStats->type != AUTOMATON )
				{
					// enemies.
					result = true;
				}
				else
				{
					switch ( myStats->type )
					{
						case SKELETON:
							if ( yourStats->type == GHOUL )
							{
								result = false;
							}
							break;
						case RAT:
							if ( yourStats->type == RAT )
							{
								result = false;
							}
							break;
						case SPIDER:
							if ( yourStats->type == SPIDER
								|| yourStats->type == SCARAB || yourStats->type == SCORPION )
							{
								result = false;
							}
							break;
						case TROLL:
							if ( yourStats->type == TROLL )
							{
								result = false;
							}
							break;
						case CREATURE_IMP:
							if ( yourStats->type == CREATURE_IMP )
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
							if ( yourStats->type == SCARAB || yourStats->type == INSECTOID || yourStats->type == SCORPION )
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
							if ( yourStats->type == INCUBUS || yourStats->type == SUCCUBUS )
							{
								result = false;
							}
							break;
						default:
							break;
					}
				}
			}
			else if ( behavior == &actMonster && your->behavior == &actPlayer && yourStats->type != HUMAN )
			{
				result = swornenemies[myStats->type][HUMAN];
				if ( (myStats->type == HUMAN || myStats->type == SHOPKEEPER) && yourStats->type != AUTOMATON )
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
						case RAT:
							if ( myStats->type == RAT )
							{
								result = false;
							}
							break;
						case SPIDER:
							if ( myStats->type == SPIDER
								|| myStats->type == SCARAB || myStats->type == SCORPION )
							{
								result = false;
							}
							break;
						case TROLL:
							if ( myStats->type == TROLL )
							{
								result = false;
							}
							break;
						case CREATURE_IMP:
							if ( myStats->type == CREATURE_IMP )
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
							if ( myStats->type == SCARAB 
								|| myStats->type == INSECTOID || myStats->type == SCORPION )
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
							if ( myStats->type == INCUBUS || myStats->type == SUCCUBUS )
							{
								result = false;
							}
							break;
						default:
							break;
					}
				}
			}

			if ( result )
			{
				if ( !(behavior == &actPlayer && effectShapeshift != NOTHING)
					&& myStats->mask && (myStats->mask->type == MASK_MASQUERADE || myStats->mask->type == MASK_MOUTH_ROSE)
					&& (yourStats->type == INCUBUS || yourStats->type == SUCCUBUS) && your->behavior == &actMonster )
				{
					result = false;
				}
				else if ( !(your->behavior == &actPlayer && your->effectShapeshift != NOTHING)
					&& yourStats->mask && (yourStats->mask->type == MASK_MASQUERADE || yourStats->mask->type == MASK_MOUTH_ROSE)
					&& (myStats->type == INCUBUS || myStats->type == SUCCUBUS) && behavior == &actMonster )
				{
					result = false;
				}
				else if ( !(behavior == &actPlayer && effectShapeshift != NOTHING)
					&& myStats->mask && myStats->mask->type == MASK_SPOOKY
					&& (yourStats->type == GHOUL || yourStats->type == SHADOW) && your->behavior == &actMonster )
				{
					result = false;
				}
				else if ( !(your->behavior == &actPlayer && your->effectShapeshift != NOTHING)
					&& yourStats->mask && yourStats->mask->type == MASK_SPOOKY
					&& (myStats->type == GHOUL || myStats->type == SHADOW) && behavior == &actMonster )
				{
					result = false;
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
	bool result = false;

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

	if ( everybodyfriendly || intro )   // friendly monsters mode
	{
		return true;
	}

	if ( (your->behavior == &actPlayer || your->behavior == &actPlayerLimb) && (behavior == &actPlayer || behavior == &actPlayerLimb) )
	{
		return true;
	}

	if ( behavior == &actPlayer && your->behavior == &actMonster && yourStats->monsterForceAllegiance != Stat::MONSTER_FORCE_ALLEGIANCE_NONE )
	{
		if ( yourStats->monsterForceAllegiance == Stat::MONSTER_FORCE_PLAYER_ALLY || yourStats->monsterForceAllegiance == Stat::MONSTER_FORCE_PLAYER_RECRUITABLE )
		{
			return true;
		}
		else if ( yourStats->monsterForceAllegiance == Stat::MONSTER_FORCE_PLAYER_ENEMY )
		{
			return false;
		}
	}
	else if ( your->behavior == &actPlayer && behavior == &actMonster && myStats->monsterForceAllegiance != Stat::MONSTER_FORCE_ALLEGIANCE_NONE )
	{
		if ( myStats->monsterForceAllegiance == Stat::MONSTER_FORCE_PLAYER_ALLY || myStats->monsterForceAllegiance == Stat::MONSTER_FORCE_PLAYER_RECRUITABLE )
		{
			return true;
		}
		else if ( myStats->monsterForceAllegiance == Stat::MONSTER_FORCE_PLAYER_ENEMY )
		{
			return false;
		}
	}


	if ( (myStats->type == SHOPKEEPER && myStats->MISC_FLAGS[STAT_FLAG_MYSTERIOUS_SHOPKEEP] > 0)
		|| (yourStats->type == SHOPKEEPER && yourStats->MISC_FLAGS[STAT_FLAG_MYSTERIOUS_SHOPKEEP] > 0) )
	{
		return false;
	}

	if ( myStats->type == GYROBOT )
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
	else if ( your->behavior == &actPlayer && myStats->type == CREATURE_IMP
		&& (!strncmp(map.name, "Boss", 4) || !strncmp(map.name, "Hell Boss", 9)) )
	{
		if ( this->monsterAllyGetPlayerLeader() )
		{
			return true;
		}
		return false;
	}
	else if ( behavior == &actPlayer && yourStats->type == CREATURE_IMP
		&& (!strncmp(map.name, "Boss", 4) || !strncmp(map.name, "Hell Boss", 9)) )
	{
		if ( your->monsterAllyGetPlayerLeader() )
		{
			return true;
		}
		return false;
	}
	else if ( your->behavior == &actPlayer && myStats->type == VAMPIRE && MonsterData_t::nameMatchesSpecialNPCName(*myStats, "bram kindly") )
	{
		return false;
	}
	else if ( behavior == &actPlayer && yourStats->type == VAMPIRE && MonsterData_t::nameMatchesSpecialNPCName(*yourStats, "bram kindly") )
	{
		return false;
	}
	else if ( behavior == &actMonster && myStats->type == INCUBUS && !strncmp(myStats->name, "inner demon", strlen("inner demon")) )
	{
		Entity* parentEntity = uidToEntity(this->parent);
		if ( parentEntity == your )
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else if ( behavior == &actPlayer && your->behavior == &actMonster && yourStats->type == INCUBUS && !strncmp(yourStats->name, "inner demon", strlen("inner demon")) )
	{
		Entity* parentEntity = uidToEntity(your->parent);
		if ( parentEntity == this )
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else if ( behavior == &actMonster && your->behavior == &actMonster && yourStats->type == INCUBUS && !strncmp(yourStats->name, "inner demon", strlen("inner demon")) )
	{
		Entity* illusionTauntingThisEntity = uidToEntity(static_cast<Uint32>(your->monsterIllusionTauntingThisUid));
		if ( illusionTauntingThisEntity == this )
		{
			return false;
		}
	}

	if ( behavior == &actPlayer && your->behavior == &actMonster 
		&& myStats->helmet && myStats->helmet->type == HAT_BOUNTYHUNTER )
	{
		if ( achievementObserver.playerAchievements[skill[2]].bountyTargets.find(your->getUID())
			!= achievementObserver.playerAchievements[skill[2]].bountyTargets.end() )
		{
			return false;
		}
	}
	else if ( your->behavior == &actPlayer && behavior == &actMonster
		&& yourStats->helmet && yourStats->helmet->type == HAT_BOUNTYHUNTER )
	{
		if ( achievementObserver.playerAchievements[your->skill[2]].bountyTargets.find(getUID())
			!= achievementObserver.playerAchievements[your->skill[2]].bountyTargets.end() )
		{
			return false;
		}
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
			if ( myStats->type == SHOPKEEPER && your->behavior == &actPlayer )
			{
				result = !ShopkeeperPlayerHostility.isPlayerEnemy(your->skill[2]);
			}
			else if ( yourStats->type == SHOPKEEPER && behavior == &actPlayer )
			{
				result = !ShopkeeperPlayerHostility.isPlayerEnemy(this->skill[2]);
			}
			else if ( behavior == &actPlayer && myStats->type != HUMAN )
			{
				result = monsterally[HUMAN][yourStats->type];
				if ( (yourStats->type == HUMAN || yourStats->type == SHOPKEEPER) && myStats->type != AUTOMATON )
				{
					result = false;
				}
				else
				{
					result = false;
					switch ( myStats->type )
					{
						case SKELETON:
							if ( yourStats->type == GHOUL )
							{
								result = true;
							}
							break;
						case RAT:
							if ( yourStats->type == RAT )
							{
								result = true;
							}
							break;
						case SPIDER:
							if ( yourStats->type == SPIDER || yourStats->type == SCARAB || yourStats->type == SCORPION )
							{
								result = true;
							}
							break;
						case TROLL:
							if ( yourStats->type == TROLL )
							{
								result = true;
							}
							break;
						case CREATURE_IMP:
							if ( yourStats->type == CREATURE_IMP )
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
							if ( yourStats->type == SCARAB 
								|| yourStats->type == INSECTOID || yourStats->type == SCORPION )
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
							if ( yourStats->type == SHOPKEEPER )
							{
								result = monsterally[SHOPKEEPER][AUTOMATON];
							}
							else if ( yourStats->type == HUMAN )
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
				if ( (myStats->type == HUMAN || myStats->type == SHOPKEEPER) && yourStats->type != AUTOMATON )
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
						case RAT:
							if ( myStats->type == RAT )
							{
								result = true;
							}
							break;
						case SPIDER:
							if ( myStats->type == SPIDER
								|| myStats->type == SCARAB || myStats->type == SCORPION )
							{
								result = true;
							}
							break;
						case TROLL:
							if ( myStats->type == TROLL )
							{
								result = true;
							}
							break;
						case CREATURE_IMP:
							if ( myStats->type == CREATURE_IMP )
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
							if ( myStats->type == SCARAB 
								|| myStats->type == INSECTOID || myStats->type == SCORPION )
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
							if ( myStats->type == SHOPKEEPER )
							{
								result = monsterally[SHOPKEEPER][AUTOMATON];
							}
							else if ( myStats->type == HUMAN )
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


void createMonsterEquipment(Stat* stats, BaronyRNG& rng)
{
	int itemIndex = 0;
	ItemType itemId;
	Status itemStatus;
	int itemBless;
	int itemAppearance = rng.rand();
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
					itemId = itemLevelCurve(static_cast<Category>(category - 1), 0, currentlevel, rng);
				}
				else
				{
					int randType = 0;
					if ( category == 14 )
					{
						// equipment
						randType = rng.rand() % 2;
						if ( randType == 0 )
						{
							itemId = itemLevelCurve(static_cast<Category>(WEAPON), 0, currentlevel, rng);
						}
						else if ( randType == 1 )
						{
							itemId = itemLevelCurve(static_cast<Category>(ARMOR), 0, currentlevel, rng);
						}
					}
					else if ( category == 15 )
					{
						// jewelry
						randType = rng.rand() % 2;
						if ( randType == 0 )
						{
							itemId = itemLevelCurve(static_cast<Category>(AMULET), 0, currentlevel, rng);
						}
						else
						{
							itemId = itemLevelCurve(static_cast<Category>(RING), 0, currentlevel, rng);
						}
					}
					else if ( category == 16 )
					{
						// magical
						randType = rng.rand() % 3;
						if ( randType == 0 )
						{
							itemId = itemLevelCurve(static_cast<Category>(SCROLL), 0, currentlevel, rng);
						}
						else if ( randType == 1 )
						{
							itemId = itemLevelCurve(static_cast<Category>(MAGICSTAFF), 0, currentlevel, rng);
						}
						else
						{
							itemId = itemLevelCurve(static_cast<Category>(SPELLBOOK), 0, currentlevel, rng);
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
					itemStatus = static_cast<Status>(DECREPIT + rng.rand() % 4);
				}
				else if ( itemStatus > BROKEN )
				{
					itemStatus = static_cast<Status>(itemStatus - 1); // reserved '0' for random, so '1' is decrepit... etc to '5' being excellent.
				}
				itemBless = stats->EDITOR_ITEMS[itemIndex * ITEM_SLOT_NUMPROPERTIES + 2];
				if ( itemBless == 10 )
				{
					itemBless = -2 + rng.rand() % 5;
				}
				itemCount = stats->EDITOR_ITEMS[itemIndex * ITEM_SLOT_NUMPROPERTIES + 3];
				if ( stats->EDITOR_ITEMS[itemIndex * ITEM_SLOT_NUMPROPERTIES + 4] == 1 )
				{
					itemIdentified = true;
				}
				else if ( stats->EDITOR_ITEMS[itemIndex * ITEM_SLOT_NUMPROPERTIES + 4] == 2 )
				{
					itemIdentified = rng.rand() % 2;
				}
				else
				{
					itemIdentified = false;
				}
				itemAppearance = rng.rand();
				chance = stats->EDITOR_ITEMS[itemIndex * ITEM_SLOT_NUMPROPERTIES + 5];

				if ( rng.rand() % 100 < chance )
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

void setRandomMonsterStats(Stat* stats, BaronyRNG& rng)
{
	if ( stats != nullptr )
	{
		//**************************************
		// HEALTH
		//**************************************

		if ( stats->MAXHP == stats->HP )
		{
			stats->MAXHP += rng.rand() % (stats->RANDOM_MAXHP + 1);

			if ( stats->RANDOM_MAXHP == stats->RANDOM_HP )
			{
				// if the max hp and normal hp range is the same, hp follows the roll of maxhp.
				stats->HP = stats->MAXHP;
			}
			else
			{
				// roll the current hp
				stats->HP += rng.rand() % (stats->RANDOM_HP + 1);
			}
		}
		else
		{
			// roll both ranges independently
			stats->MAXHP += rng.rand() % (stats->RANDOM_MAXHP + 1);
			stats->HP += rng.rand() % (stats->RANDOM_HP + 1);
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
			stats->MAXMP += rng.rand() % (stats->RANDOM_MAXMP + 1);

			if ( stats->RANDOM_MAXMP == stats->RANDOM_MP )
			{
				// if the max mp and normal mp range is the same, mp follows the roll of maxmp.
				stats->MP = stats->MAXMP;
			}
			else
			{
				// roll the current mp
				stats->MP += rng.rand() % (stats->RANDOM_MP + 1);
			}
		}
		else
		{
			// roll both ranges independently
			stats->MAXMP += rng.rand() % (stats->RANDOM_MAXMP + 1);
			stats->MP += rng.rand() % (stats->RANDOM_MP + 1);
		}

		if ( stats->MP > stats->MAXMP )
		{
			// check if mp exceeds maximums
			stats->MP = stats->MAXMP;
		}

		//**************************************
		// REST OF STATS
		//**************************************

		stats->STR += rng.rand() % (stats->RANDOM_STR + 1);
		stats->DEX += rng.rand() % (stats->RANDOM_DEX + 1);
		stats->CON += rng.rand() % (stats->RANDOM_CON + 1);
		stats->INT += rng.rand() % (stats->RANDOM_INT + 1);
		stats->PER += rng.rand() % (stats->RANDOM_PER + 1);
		stats->CHR += rng.rand() % (stats->RANDOM_CHR + 1);

		stats->LVL += rng.rand() % (stats->RANDOM_LVL + 1);
		stats->GOLD += rng.rand() % (stats->RANDOM_GOLD + 1);
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
	if ( !item )
	{
		return TYPE_NONE;
	}
	if ( itemTypeIsQuiver(item->type) )
	{
		return TYPE_OFFHAND;
	}
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
		case SUEDE_BOOTS:
			return TYPE_BOOTS;
			break;

		case LEATHER_HELM:
		case IRON_HELM:
		case STEEL_HELM:
		case CRYSTAL_HELM:
		case ARTIFACT_HELM:
		case HAT_BEAR_HOOD:
		case HAT_STAG_HOOD:
		case HAT_WOLF_HOOD:
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
		case CLOAK_BACKPACK:
		case CLOAK_SILVER:
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
		case SUEDE_GLOVES:
			return TYPE_GLOVES;
			break;

		case HAT_HOOD:
		case HAT_JESTER:
		case HAT_PHRYGIAN:
		case HAT_WIZARD:
		case HAT_FEZ:
		case HAT_HOOD_RED:
		case MASK_SHAMAN:
		case PUNISHER_HOOD:
		case HAT_HOOD_SILVER:
		case HAT_SILKEN_BOW:
		case HAT_PLUMED_CAP:
		case HAT_BYCOCKET:
		case HAT_TOPHAT:
		case HAT_BANDANA:
		case HAT_CIRCLET:
		case HAT_CROWN:
		case HAT_LAURELS:
		case HAT_TURBAN:
		case HAT_WARM:
		case HAT_BOUNTYHUNTER:
		case HAT_MITER:
		case HAT_HEADDRESS:
		case HAT_CHEF:
		case HELM_MINING:
		case HAT_CIRCLET_WISDOM:
		case HAT_BUNNY_HOOD:
		case HAT_HOOD_APPRENTICE:
		case HAT_HOOD_ASSASSIN:
		case HAT_HOOD_WHISPERS:
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
	else if ( myStats->gloves->type == SUEDE_GLOVES )
	{
		ent->sprite = 804 + (spriteOffset > 0 ? 1 : 0);
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
			else if ( myStats->shoes->type == SUEDE_BOOTS )
			{
				leg->sprite = 808 + (spriteOffset > 0 ? 1 : 0);
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
			else if ( myStats->shoes->type == SUEDE_BOOTS )
			{
				leg->sprite = 808 + (spriteOffset > 0 ? 1 : 0);
			}
			else
			{
				return false;
			}
			break;
		case GNOME:
			if ( myStats->shoes->type == LEATHER_BOOTS || myStats->shoes->type == LEATHER_BOOTS_SPEED )
			{
				leg->sprite = 1463 + (spriteOffset > 0 ? 1 : 0);
			}
			else if ( myStats->shoes->type == IRON_BOOTS || myStats->shoes->type == IRON_BOOTS_WATERWALKING )
			{
				leg->sprite = 1467 + (spriteOffset > 0 ? 1 : 0);
			}
			else if ( myStats->shoes->type >= STEEL_BOOTS && myStats->shoes->type <= STEEL_BOOTS_FEATHER )
			{
				leg->sprite = 1471 + (spriteOffset > 0 ? 1 : 0);
			}
			else if ( myStats->shoes->type == CRYSTAL_BOOTS )
			{
				leg->sprite = 1465 + (spriteOffset > 0 ? 1 : 0);
			}
			else if ( myStats->shoes->type == ARTIFACT_BOOTS )
			{
				leg->sprite = 1461 + (spriteOffset > 0 ? 1 : 0);
			}
			else if ( myStats->shoes->type == SUEDE_BOOTS )
			{
				leg->sprite = 1473 + (spriteOffset > 0 ? 1 : 0);
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
					if ( mystats->type == CREATURE_IMP )
					{
						return true;
					}
					return false;
				}
			}
		}
	}
	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		if ( players[i] && players[i]->entity )
		{
			if ( players[i]->entity->getStats() == mystats )
			{
				if ( players[i]->entity->effectShapeshift == CREATURE_IMP )
				{
					return true;
				}
				break;
			}
		}
	}

	if ( mystats->EFFECTS[EFF_LEVITATING] == true )
	{
		return true;
	}
	else if ( mystats->EFFECTS[EFF_FLUTTER] )
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
		return PRO_UNARMED;
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
	if ( isRangedWeapon(*weapon) )
	{
		return PRO_RANGED;
	}
	if ( itemCategory(weapon) == THROWN || itemCategory(weapon) == POTION || itemCategory(weapon) == GEM )
	{
		return PRO_RANGED;
	}
	if ( weapon->type == TOOL_WHIP )
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
		case PRO_UNARMED:
			statForProficiency = STAT_STR;
			break;
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
		case PRO_ALCHEMY:       // base attribute: int
			statForProficiency = STAT_INT;
			break;
		case PRO_LOCKPICKING:	// base attribute: per
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

void Entity::setMeleeDamageSkillModifiers(Entity* my, Stat* myStats, int skill, real_t& baseSkillModifier, real_t& variance, ItemType* itemType)
{
	bool shapeshifted = (my && my->behavior == &actPlayer && my->effectShapeshift != NOTHING);
	bool gungnir = false;

	variance = 20;
	baseSkillModifier = 50.0; // 40-60 base

	if ( !shapeshifted )
	{
		if ( myStats && myStats->weapon && myStats->weapon->type == ARTIFACT_SPEAR )
		{
			gungnir = true;
		}
	}
	if ( itemType && (*itemType) == ARTIFACT_SPEAR )
	{
		gungnir = true;
	}

	if ( skill == PRO_UNARMED )
	{
		variance = 30.0;
		baseSkillModifier = 45.0; // 30-60 base
	}
	else if ( skill == PRO_POLEARM )
	{
		if ( gungnir )
		{
			variance = 0.0;
			baseSkillModifier = 60.0;
		}
		else
		{
			variance = 10.0;
			baseSkillModifier = 55.0; // 50-60 base
		}
	}
	return;
}


int Entity::isEntityPlayer() const
{
	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		if ( players[i] && this == players[i]->entity )
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
		else if ( myStats->type == SENTRYBOT )
		{
			pose = MONSTER_POSE_RANGED_WINDUP1;
		}
		else if ( myStats->type == SPELLBOT )
		{
			pose = MONSTER_POSE_MAGIC_WINDUP1;
		}
		else if ( myStats->type == MIMIC )
		{
			pose = MONSTER_POSE_MELEE_WINDUP1;
			if ( monsterSpecialState == MIMIC_MAGIC )
			{
				pose = MONSTER_POSE_MIMIC_MAGIC1;
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
				|| myStats->type == SPIDER
				|| myStats->type == CRAB
				|| myStats->type == SHOPKEEPER || myStats->type == SHADOW )
			{
				pose = MONSTER_POSE_MAGIC_WINDUP1;
			}
			else if ( myStats->type == DEMON || myStats->type == CREATURE_IMP || myStats->type == GHOUL )
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
				|| myStats->type == BUGBEAR
				|| myStats->type == SHADOW )
			{
				if ( myStats->weapon->type == CROSSBOW || myStats->weapon->type == HEAVY_CROSSBOW )
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
				|| myStats->type == BUGBEAR
				|| myStats->type == SHADOW )
			{
				if ( getWeaponSkill(myStats->weapon) == PRO_AXE || getWeaponSkill(myStats->weapon) == PRO_MACE
					|| myStats->weapon->type == TOOL_WHIP )
				{
					// axes and maces don't stab
					pose = MONSTER_POSE_MELEE_WINDUP1 + local_rng.rand() % 2;
				}
				else
				{
					pose = MONSTER_POSE_MELEE_WINDUP1 + local_rng.rand() % 3;
				}
			}
			else
			{
				pose = local_rng.rand() % 3 + 1;
			}
		}
	}
	// fists
	else
	{
	    const auto type = myStats->type;
		if ( myStats->type == SLIME && this->monsterSpecialTimer == MONSTER_SPECIAL_COOLDOWN_SLIME_SPRAY )
		{
			pose = MONSTER_POSE_MAGIC_WINDUP2;
		}
		else if (type == KOBOLD || type == AUTOMATON ||
			type == GOATMAN || type == INSECTOID ||
			type == INCUBUS || type == VAMPIRE ||
			type == HUMAN || type == GOBLIN ||
			type == GHOUL || type == SKELETON ||
			type == GNOME || type == DEMON ||
			type == CREATURE_IMP || type == SUCCUBUS ||
			type == SHOPKEEPER || type == MINOTAUR ||
			type == SHADOW || type == RAT || type == SPIDER || type == CRAB ||
			type == MIMIC || type == BAT_SMALL ||
			type == SLIME || (type == SCARAB && sprite != 1078 && sprite != 1079))
		{
			pose = MONSTER_POSE_MELEE_WINDUP1;
			if ( type == MIMIC && monsterSpecialState == MIMIC_MAGIC )
			{
				pose = MONSTER_POSE_MIMIC_MAGIC1;
			}
		}
		else if ( myStats->type == CRYSTALGOLEM )
		{
			if ( this->monsterSpecialTimer == MONSTER_SPECIAL_COOLDOWN_GOLEM )
			{
				pose = MONSTER_POSE_MELEE_WINDUP3;
			}
			else
			{
				pose = MONSTER_POSE_MELEE_WINDUP1 + local_rng.rand() % 2;
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
				pose = MONSTER_POSE_MELEE_WINDUP1 + local_rng.rand() % 2;
			}
		}
		else if ( myStats->type == TROLL )
		{
			pose = MONSTER_POSE_MELEE_WINDUP1;
		}
		else if ( myStats->type == BUGBEAR )
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

	if ( isRangedWeapon(*myStats->weapon) )
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
			if ( dist > 0.1 && (bodypart != LIMB_HUMANOID_LEFTARM || shield->sprite <= 0) )
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
		{
			static std::vector<int> leatherSteps{ 0, 3, 4, 5, 6 };
			sound = leatherSteps[local_rng.rand() % leatherSteps.size()];
			break;
		}
		case MONSTER_FOOTSTEP_USE_BOOTS:
		{
			if ( bootSprite >= 152 && bootSprite <= 155 ) // iron boots
			{
				static std::vector<int> ironSteps{ 7, 10, 11, 12, 13 };
				sound = ironSteps[local_rng.rand() % ironSteps.size()];
			}
			else if ( (bootSprite >= 156 && bootSprite <= 159) // steel boots
				|| (bootSprite >= 499 && bootSprite <= 502) // crystal boots
				|| (bootSprite >= 521 && bootSprite <= 524) ) // artifact boots
			{
				static std::vector<int> steelSteps{ 14, 15, 16, 17, 18, 19, 20 };
				sound = steelSteps[local_rng.rand() % steelSteps.size()];
			}
			else
			{
				static std::vector<int> defaultSteps{ 0, 3, 4, 5, 6 };
				sound = defaultSteps[local_rng.rand() % defaultSteps.size()];
			}
			break;
		}
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

	if ( weaponLimb->flags[INVISIBLE] == false || weaponLimb->flags[INVISIBLE_DITHER] ) //TODO: isInvisible()?
	{
		if ( weaponLimb->sprite == items[SHORTBOW].index )
		{
			weaponLimb->x = weaponArmLimb->x - .5 * cos(weaponArmLimb->yaw);
			weaponLimb->y = weaponArmLimb->y - .5 * sin(weaponArmLimb->yaw);
			weaponLimb->z = weaponArmLimb->z + 1;
			weaponLimb->pitch = weaponArmLimb->pitch + .25;
		}
		else if ( weaponLimb->sprite == items[ARTIFACT_BOW].index 
			|| weaponLimb->sprite == items[LONGBOW].index
			|| weaponLimb->sprite == items[COMPOUND_BOW].index )
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
				weaponLimb->x = weaponArmLimb->x - .5 * cos(weaponArmLimb->yaw);
				weaponLimb->y = weaponArmLimb->y - .5 * sin(weaponArmLimb->yaw);
				weaponLimb->z = weaponArmLimb->z + 1;
				weaponLimb->pitch = weaponArmLimb->pitch + .25;
			}

			if ( weaponLimb->sprite == items[LONGBOW].index )
			{
				weaponLimb->x -= .5 * cos(weaponArmLimb->yaw);
				weaponLimb->y -= .5 * sin(weaponArmLimb->yaw);
			}
			else if ( weaponLimb->sprite == items[COMPOUND_BOW].index )
			{
				weaponLimb->x += .5 * cos(weaponArmLimb->yaw);
				weaponLimb->y += .5 * sin(weaponArmLimb->yaw);
			}
		}
		else if ( weaponLimb->sprite == items[CROSSBOW].index || weaponLimb->sprite == items[HEAVY_CROSSBOW].index )
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
					real_t forward = 3.0;
					if ( monsterType == BUGBEAR )
					{
						forward = limbs[BUGBEAR][18][0];
					}

					weaponLimb->x = weaponArmLimb->x + (forward * cos(weaponArmLimb->pitch)) * cos(weaponArmLimb->yaw);
					weaponLimb->y = weaponArmLimb->y + (forward * cos(weaponArmLimb->pitch)) * sin(weaponArmLimb->yaw);

					if ( weaponArmLimb->pitch < PI / 3 )
					{
						// adjust the z point halfway through swing.
						if ( monsterType == BUGBEAR )
						{
							weaponLimb->z = weaponArmLimb->z;
							weaponLimb->z += limbs[BUGBEAR][18][1] * sin(weaponArmLimb->pitch * 2);
						}
						else
						{
							weaponLimb->z = weaponArmLimb->z + 1.5 - 2 * cos(weaponArmLimb->pitch / 2);
						}
						if ( monsterType == INCUBUS || monsterType == SUCCUBUS )
						{
							weaponLimb->z += 2;
						}
					}
					else
					{
						if ( monsterType == BUGBEAR )
						{
							weaponLimb->z = weaponArmLimb->z;
							weaponLimb->z += limbs[BUGBEAR][18][2] * sin(weaponArmLimb->pitch * 2);
						}
						else
						{
							weaponLimb->z = weaponArmLimb->z - .5 * (myAttack == 0);
						}
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
					if ( monsterType == BUGBEAR )
					{
						weaponLimb->z = weaponArmLimb->z;
					}
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
				if ( monsterType == BUGBEAR )
				{
					if ( !isPlayer && this->monsterArmbended )
					{
						weaponLimb->x = weaponArmLimb->x;
						weaponLimb->y = weaponArmLimb->y;
						weaponLimb->z = weaponArmLimb->z;
						weaponLimb->pitch = weaponArmLimb->pitch;
					}
				}
			}
		}
	}

	weaponLimb->yaw = weaponArmLimb->yaw;
	bool isPotion = false;
	if ( myAttack == MONSTER_POSE_RANGED_WINDUP3 && monsterType == GOATMAN && !isPlayer )
	{
		// specific for potion throwing goatmen.
		limbAnimateToLimit(weaponLimb, ANIMATE_ROLL, 0.25, 1 * PI / 4, false, 0.0);
	}
	else
	{
		weaponLimb->roll = weaponArmLimb->roll;
		if ( isPlayer )
		{
			if ( (weaponLimb->sprite >= 50 && weaponLimb->sprite < 58)
				|| weaponLimb->sprite == 795 )
			{
				weaponLimb->roll += (PI / 2); // potion sprites rotated
				isPotion = true;
			}
			else if ( weaponLimb->sprite == items[BOOMERANG].index )
			{
				weaponLimb->roll += (PI / 2); // sprite rotated
				weaponLimb->pitch -= PI / 8;
				weaponLimb->pitch += .25 * (myAttack != 0); // add 0.25 if attacking
			}
			else if ( weaponLimb->sprite == items[FOOD_CREAMPIE].index )
			{
				weaponLimb->roll += (PI / 2); // sprite rotated
			}
		}
	}

	bool armBended = (!isPlayer && this->monsterArmbended) || (isPlayer && this->skill[11]);
	weaponLimb->scalex = 1.f;
	weaponLimb->scaley = 1.f;
	weaponLimb->scalez = 1.f;
	if ( weaponLimb->sprite == items[TOOL_WHIP].index || weaponLimb->sprite == items[TOOL_WHIP].index + 1 )
	{
		if ( myAttack != 2 )
		{
			weaponLimb->pitch -= PI / 8;
			if ( weaponLimb->sprite == items[TOOL_WHIP].index + 1 )
			{
				weaponLimb->pitch -= PI / 8;
			}
		}
		if ( myAttack == 1 )
		{
			if ( weaponArmLimb->skill[1] == 1 && armBended )
			{
				if ( weaponArmLimb->pitch >= 3 * PI / 2 )
				{
					if ( weaponLimb->sprite == items[TOOL_WHIP].index )
					{
						weaponLimb->sprite += 1;
					}
				}
				else if ( weaponArmLimb->pitch >= PI / 10 )
				{
					if ( weaponLimb->sprite == items[TOOL_WHIP].index + 1 )
					{
						weaponLimb->sprite = items[TOOL_WHIP].index;
					}
				}
			}
			else
			{
				weaponLimb->sprite = items[TOOL_WHIP].index;
			}
		}
		else
		{
			weaponLimb->sprite = items[TOOL_WHIP].index;
		}
	}
	else if ( weaponLimb->sprite == items[TOOL_DECOY].index || weaponLimb->sprite == items[TOOL_DUMMYBOT].index )
	{
		weaponLimb->scalex = 0.8;
		weaponLimb->scaley = 0.8;
		weaponLimb->scalez = 0.8;
	}

	if ( isPlayer && monsterType == CREATURE_IMP )
	{
		weaponLimb->focalx = limbs[monsterType][9][0];
		weaponLimb->focaly = limbs[monsterType][9][1];
		weaponLimb->focalz = limbs[monsterType][9][2];
		weaponLimb->pitch += .5 + limbs[monsterType][10][0];
	}
	else if ( !armBended )
	{
		weaponLimb->focalx = limbs[monsterType][6][0]; // 2.5
		weaponLimb->focaly = limbs[monsterType][6][1]; // 0
		if ( weaponLimb->sprite == items[CROSSBOW].index || weaponLimb->sprite == items[HEAVY_CROSSBOW].index )
		{
			weaponLimb->focalx += 2.1;
			weaponLimb->focaly -= 0.1;
		}
		weaponLimb->focalz = limbs[monsterType][6][2]; // -.5
		if ( isPlayer && isPotion )
		{
			weaponLimb->focalz += 1;
			if ( monsterType == INCUBUS || monsterType == SUCCUBUS )
			{
				weaponLimb->focaly += 1;
				weaponLimb->focalz -= 1.5;
			}
		}
		else if ( weaponLimb->sprite == items[BOOMERANG].index )
		{
			weaponLimb->focalx += 2;
			weaponLimb->focaly += 0.25;
			weaponLimb->focalz += 0;
			weaponLimb->x += -1.2 * cos(weaponArmLimb->yaw + PI / 2) + -.6 * cos(weaponArmLimb->yaw);
			weaponLimb->y += -1.2 * sin(weaponArmLimb->yaw + PI / 2) + -.6 * sin(weaponArmLimb->yaw);
			weaponLimb->z += 0.25;
			switch ( monsterType )
			{
				case SKELETON:
				case AUTOMATON:
				case GOATMAN:
				case INSECTOID:
				case GOBLIN:
					weaponLimb->x += 0.5 * cos(weaponArmLimb->yaw + PI / 2);
					weaponLimb->y += 0.5 * sin(weaponArmLimb->yaw + PI / 2);
					break;
				case INCUBUS:
				case SUCCUBUS:
					weaponLimb->x += 1.75 * cos(weaponArmLimb->yaw + PI / 2) + 0.25 * cos(weaponArmLimb->yaw);
					weaponLimb->y += 1.75 * sin(weaponArmLimb->yaw + PI / 2) + 0.25 * sin(weaponArmLimb->yaw);
					weaponLimb->z += 0;

					weaponLimb->focalx += -0.75;
					weaponLimb->focaly += 1;
					weaponLimb->focalz += 0;
					break;
				default:
					break;
			}
		}
		else if ( weaponLimb->sprite == items[TOOL_WHIP].index || weaponLimb->sprite == items[TOOL_WHIP].index + 1 )
		{
			weaponLimb->focalx += 1;
			weaponLimb->focalz += 1.5;
		}
		else if ( weaponLimb->sprite == items[TOOL_GYROBOT].index )
		{
			weaponLimb->focalz += 1;
		}
		else if ( weaponLimb->sprite >= items[TOOL_BOMB].index && weaponLimb->sprite <= items[TOOL_TELEPORT_BOMB].index )
		{
			weaponLimb->focalz += 2;
			if ( monsterType == SKELETON )
			{
				weaponLimb->focalx -= 0.25;
				weaponLimb->focaly += 0.1;
			}
			else if ( monsterType == AUTOMATON )
			{
				weaponLimb->focaly += 0.5;
				weaponLimb->focalz -= 1;
			}
		}
		else if ( weaponLimb->sprite == items[SHORTBOW].index || weaponLimb->sprite == items[ARTIFACT_BOW].index
			|| weaponLimb->sprite == items[LONGBOW].index || weaponLimb->sprite == items[COMPOUND_BOW].index )
		{
			if ( weaponLimb->sprite == items[SHORTBOW].index )
			{
				switch ( monsterType )
				{
					case HUMAN:
					case VAMPIRE:
					case SHOPKEEPER:
						weaponLimb->x += -.1 * cos(weaponArmLimb->yaw + PI / 2) + 0.25 * cos(weaponArmLimb->yaw);
						weaponLimb->y += -.1 * sin(weaponArmLimb->yaw + PI / 2) + 0.25 * sin(weaponArmLimb->yaw);
						weaponLimb->z += -1.25;
						weaponLimb->focalx += -0.5;
						weaponLimb->focaly += 0;
						weaponLimb->focalz += 1.75;
						break;
					case GOBLIN:
					case GOATMAN:
					case INSECTOID:
					case SUCCUBUS:
					case INCUBUS:
						weaponLimb->x += -.1 * cos(weaponArmLimb->yaw + PI / 2) + 0.25 * cos(weaponArmLimb->yaw);
						weaponLimb->y += -.1 * sin(weaponArmLimb->yaw + PI / 2) + 0.25 * sin(weaponArmLimb->yaw);
						weaponLimb->z += -1;
						weaponLimb->focalx += 0;
						weaponLimb->focaly += 0;
						weaponLimb->focalz += 1.25;
						break;
					case SKELETON:
					case AUTOMATON:
						weaponLimb->x += -.1 * cos(weaponArmLimb->yaw + PI / 2) + 0.5 * cos(weaponArmLimb->yaw);
						weaponLimb->y += -.1 * sin(weaponArmLimb->yaw + PI / 2) + 0.5 * sin(weaponArmLimb->yaw);
						weaponLimb->z += -1;
						weaponLimb->focalx += -0.5;
						weaponLimb->focaly += 0;
						weaponLimb->focalz += 1;
						break;
					default:
						break;
				}
			}
			else if ( weaponLimb->sprite == items[ARTIFACT_BOW].index
				|| weaponLimb->sprite == items[LONGBOW].index )
			{
				switch ( monsterType )
				{
					case HUMAN:
					case VAMPIRE:
					case SHOPKEEPER:
						weaponLimb->x += -.1 * cos(weaponArmLimb->yaw + PI / 2) + 0.75 * cos(weaponArmLimb->yaw);
						weaponLimb->y += -.1 * sin(weaponArmLimb->yaw + PI / 2) + 0.75 * sin(weaponArmLimb->yaw);
						weaponLimb->z += -1.25;
						weaponLimb->focalx += -0.5;
						weaponLimb->focaly += 0;
						weaponLimb->focalz += 1.75;
						if ( weaponLimb->sprite == items[LONGBOW].index )
						{
							weaponLimb->x += -0.25 * cos(weaponArmLimb->yaw);
							weaponLimb->y += -0.25 * sin(weaponArmLimb->yaw);
							weaponLimb->z += 0.25;
							weaponLimb->focalx += 0.5;
							weaponLimb->focaly += 0;
							weaponLimb->focalz += -0.5;
						}
						break;
					case GOBLIN:
					case GOATMAN:
					case INSECTOID:
						weaponLimb->x += -.1 * cos(weaponArmLimb->yaw + PI / 2) + 0.5 * cos(weaponArmLimb->yaw);
						weaponLimb->y += -.1 * sin(weaponArmLimb->yaw + PI / 2) + 0.5 * sin(weaponArmLimb->yaw);
						weaponLimb->z += -1;
						weaponLimb->focalx += 0;
						weaponLimb->focaly += 0;
						weaponLimb->focalz += 1.25;
						break;
					case SUCCUBUS:
					case INCUBUS:
						weaponLimb->x += -.1 * cos(weaponArmLimb->yaw + PI / 2) + 0.25 * cos(weaponArmLimb->yaw);
						weaponLimb->y += -.1 * sin(weaponArmLimb->yaw + PI / 2) + 0.25 * sin(weaponArmLimb->yaw);
						weaponLimb->z += -1;
						weaponLimb->focalx += 0;
						weaponLimb->focaly += 0;
						weaponLimb->focalz += 1.25;
						break;
					case SKELETON:
					case AUTOMATON:
						weaponLimb->x += -.1 * cos(weaponArmLimb->yaw + PI / 2) + 0.25 * cos(weaponArmLimb->yaw);
						weaponLimb->y += -.1 * sin(weaponArmLimb->yaw + PI / 2) + 0.25 * sin(weaponArmLimb->yaw);
						weaponLimb->z += -1.25;
						weaponLimb->focalx += 0;
						weaponLimb->focaly += 0;
						weaponLimb->focalz += 1.25;
						break;
					default:
						break;
				}
				if ( weaponLimb->sprite == items[LONGBOW].index )
				{
					// this applies to all offsets for all monsters.
					weaponLimb->x += -.1 * cos(weaponArmLimb->yaw + PI / 2) + 0.75 * cos(weaponArmLimb->yaw);
					weaponLimb->y += -.1 * sin(weaponArmLimb->yaw + PI / 2) + 0.75 * sin(weaponArmLimb->yaw);
					weaponLimb->z += 0.25;
					weaponLimb->focalx += -.75;
					weaponLimb->focaly += 0;
					weaponLimb->focalz += 0;
				}
			}
			else if ( weaponLimb->sprite == items[COMPOUND_BOW].index )
			{
				switch ( monsterType )
				{
					case HUMAN:
					case VAMPIRE:
					case SHOPKEEPER:
						weaponLimb->x += -.1 * cos(weaponArmLimb->yaw + PI / 2) + 0.f * cos(weaponArmLimb->yaw);
						weaponLimb->y += -.1 * sin(weaponArmLimb->yaw + PI / 2) + 0.f * sin(weaponArmLimb->yaw);
						weaponLimb->z += -1.25;
						weaponLimb->focalx += 0;
						weaponLimb->focaly += 0;
						weaponLimb->focalz += 1.75;
						break;
					case GOBLIN:
					case GOATMAN:
					case INSECTOID:
						weaponLimb->x += -.1 * cos(weaponArmLimb->yaw + PI / 2) + 0.5 * cos(weaponArmLimb->yaw);
						weaponLimb->y += -.1 * sin(weaponArmLimb->yaw + PI / 2) + 0.5 * sin(weaponArmLimb->yaw);
						weaponLimb->z += -1;
						weaponLimb->focalx += 0;
						weaponLimb->focaly += 0;
						weaponLimb->focalz += 1.25;
						break;
					case SUCCUBUS:
					case INCUBUS:
						weaponLimb->x += -.1 * cos(weaponArmLimb->yaw + PI / 2) + 0.25 * cos(weaponArmLimb->yaw);
						weaponLimb->y += -.1 * sin(weaponArmLimb->yaw + PI / 2) + 0.25 * sin(weaponArmLimb->yaw);
						weaponLimb->z += -1;
						weaponLimb->focalx += 0;
						weaponLimb->focaly += 0;
						weaponLimb->focalz += 1.25;
						break;
					case SKELETON:
					case AUTOMATON:
						weaponLimb->x += -.1 * cos(weaponArmLimb->yaw + PI / 2) + 0.25 * cos(weaponArmLimb->yaw);
						weaponLimb->y += -.1 * sin(weaponArmLimb->yaw + PI / 2) + 0.25 * sin(weaponArmLimb->yaw);
						weaponLimb->z += -1.25;
						weaponLimb->focalx += 0;
						weaponLimb->focaly += 0;
						weaponLimb->focalz += 1.25;
						break;
					default:
						break;
				}
			}
			/*weaponLimb->x += limbs[HUMAN][12][0] * cos(weaponArmLimb->yaw + PI / 2) + limbs[HUMAN][12][1] * cos(weaponArmLimb->yaw);
			weaponLimb->y += limbs[HUMAN][12][0] * sin(weaponArmLimb->yaw + PI / 2) + limbs[HUMAN][12][1] * sin(weaponArmLimb->yaw);
			weaponLimb->z += limbs[HUMAN][12][2];
			weaponLimb->focalx += limbs[HUMAN][11][0];
			weaponLimb->focaly += limbs[HUMAN][11][1];
			weaponLimb->focalz += limbs[HUMAN][11][2];*/
		}
		else
		{
			switch ( monsterType )
			{
				case SUCCUBUS:
				case INCUBUS:
				case HUMAN:
				case VAMPIRE:
				case AUTOMATON:
				case INSECTOID:
				case GOBLIN:
					weaponLimb->focaly -= 0.05; // minor z-fighting fix.
					break;
				default:
					break;
			}
		}
	}
	else
	{
		if ( monsterType == INCUBUS || monsterType == SUCCUBUS )
		{
			weaponLimb->focalx = limbs[monsterType][6][0] + 2; // 3.5
			weaponLimb->focaly = limbs[monsterType][6][1]; // 0
			weaponLimb->focalz = limbs[monsterType][6][2] - 3.5; // -2.5
			if ( isPlayer && isPotion )
			{
				weaponLimb->focalz += 4.5;
			}
		}
		else if ( isPlayer && monsterType == HUMAN )
		{
			weaponLimb->focalx = limbs[monsterType][6][0] + 1.5;
			weaponLimb->focaly = limbs[monsterType][6][1];
			weaponLimb->focalz = limbs[monsterType][6][2] - 2;
			if ( isPlayer && isPotion )
			{
				weaponLimb->focalz += 3;
			}
		}
		else
		{
			weaponLimb->focalx = limbs[monsterType][6][0] + 1; // 3.5
			weaponLimb->focaly = limbs[monsterType][6][1]; // 0
			weaponLimb->focalz = limbs[monsterType][6][2] - 2; // -2.5
			if ( isPlayer && isPotion )
			{
				weaponLimb->focalz += 3;
			}
		}

		if ( weaponLimb->sprite == items[BOOMERANG].index )
		{
			weaponLimb->focalx += 2;
			weaponLimb->focaly += 0.25;
			weaponLimb->focalz += 2;
			weaponLimb->x += -1.2 * cos(weaponArmLimb->yaw + PI / 2) + -.1 * cos(weaponArmLimb->yaw);
			weaponLimb->y += -1.2 * sin(weaponArmLimb->yaw + PI / 2) + -.1 * sin(weaponArmLimb->yaw);
			weaponLimb->z += 0.25;
			switch ( monsterType )
			{
				case SKELETON:
				case AUTOMATON:
				case GOATMAN:
				case INSECTOID:
				case GOBLIN:
					weaponLimb->x += 0.5 * cos(weaponArmLimb->yaw + PI / 2);
					weaponLimb->y += 0.5 * sin(weaponArmLimb->yaw + PI / 2);
					break;
				case INCUBUS:
				case SUCCUBUS:
					weaponLimb->x += 1.75 * cos(weaponArmLimb->yaw + PI / 2) + 0.25 * cos(weaponArmLimb->yaw);
					weaponLimb->y += 1.75 * sin(weaponArmLimb->yaw + PI / 2) + 0.25 * sin(weaponArmLimb->yaw);
					weaponLimb->z += 0;

					weaponLimb->focalx += -0.75;
					weaponLimb->focaly += 1;
					weaponLimb->focalz += 1.5;
					break;
				default:
					break;
			}
		}
		else if ( weaponLimb->sprite == items[TOOL_WHIP].index + 1 )
		{
			weaponLimb->focalx += 5.5;
			weaponLimb->focalz += 3.5;
		}
		else if ( weaponLimb->sprite == items[TOOL_WHIP].index )
		{
			weaponLimb->focalx += 1.5;
			weaponLimb->focalz += 2.5;
		}
		else if ( weaponLimb->sprite == items[TOOL_GYROBOT].index )
		{
			weaponLimb->focalz += 1;
		}
		else if ( weaponLimb->sprite >= items[TOOL_BOMB].index && weaponLimb->sprite <= items[TOOL_TELEPORT_BOMB].index )
		{
			weaponLimb->focalz += 2;
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
	monsterMoveTime = local_rng.rand() % 10 + 1;
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

void actAmbientParticleEffectIdle2(Entity* my)
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
		my->yaw += 0.1;
		if ( my->yaw > 2 * PI )
		{
			my->yaw = 0;
		}
	}
	return;
}

void Entity::spawnAmbientParticles2(int chance, int particleSprite, int duration, double particleScale, bool shrink)
{
	if ( local_rng.rand() % chance == 0 )
	{
		Entity* spawnParticle = newEntity(particleSprite, 1, map.entities, nullptr); //Particle entity.
		spawnParticle->sizex = 1;
		spawnParticle->sizey = 1;
		spawnParticle->x = x + (-2 + local_rng.rand() % 5);
		spawnParticle->y = y + (-2 + local_rng.rand() % 5);
		spawnParticle->z = z - 2;
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
		spawnParticle->behavior = &actAmbientParticleEffectIdle2;
		spawnParticle->flags[PASSABLE] = true;
		spawnParticle->setUID(-3);
	}
}

void Entity::spawnAmbientParticles(int chance, int particleSprite, int duration, double particleScale, bool shrink)
{
	if ( local_rng.rand() % chance == 0 )
	{
		Entity* spawnParticle = newEntity(particleSprite, 1, map.entities, nullptr); //Particle entity.
		spawnParticle->sizex = 1;
		spawnParticle->sizey = 1;
		spawnParticle->x = x + (-2 + local_rng.rand() % 5);
		spawnParticle->y = y + (-2 + local_rng.rand() % 5);
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
		spawnAmbientParticles(80, 579, 10 + local_rng.rand() % 40, 1.0, false);
	}

	if ( myStats->EFFECTS[EFF_FEAR] )
	{
		if ( ticks % 25 == 0 || ticks % 40 == 0 )
		{
			spawnAmbientParticles(1, 864, 20 + local_rng.rand() % 10, 0.5, true);
		}
	}

	if ( myStats->EFFECTS[EFF_TROLLS_BLOOD] )
	{
		spawnAmbientParticles(80, 169, 20 + local_rng.rand() % 10, 0.5, true);
	}

	if ( myStats->EFFECTS[EFF_VAMPIRICAURA] )
	{
		spawnAmbientParticles(30, 600, 20 + local_rng.rand() % 30, 0.5, true);
	}

	if ( myStats->EFFECTS[EFF_PACIFY] )
	{
		spawnAmbientParticles(30, 685, 20 + local_rng.rand() % 30, 0.5, true);
	}

	if ( myStats->EFFECTS[EFF_SHADOW_TAGGED] )
	{
		if ( ticks % 25 == 0 || ticks % 40 == 0 )
		{
			spawnAmbientParticles(1, 871, 20 + local_rng.rand() % 10, 0.5, true);
		}
	}

	if ( myStats->EFFECTS[EFF_POLYMORPH] )
	{
		if ( ticks % 25 == 0 || ticks % 40 == 0 )
		{
			spawnAmbientParticles(1, 593, 20 + local_rng.rand() % 10, 0.5, true);
		}
	}

	if ( myStats->EFFECTS[EFF_INVISIBLE] && getMonsterTypeFromSprite() == SHADOW )
	{
		spawnAmbientParticles(20, 175, 20 + local_rng.rand() % 30, 0.5, true);
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

	for ( int player = 1; player < MAXPLAYERS; ++player )
	{
		if ( client_disconnected[player] || players[player]->isLocalPlayer() )
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
		net_packet->data[12] = 0;
		for ( int i = 0; i < NUMEFFECTS; ++i )
		{
			if ( myStats->EFFECTS[i] )
			{
				net_packet->data[8 + i / 8] |= power(2, i - (i / 8) * 8);
			}
		}
		net_packet->address.host = net_clients[player - 1].host;
		net_packet->address.port = net_clients[player - 1].port;
		net_packet->len = 13;
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

	if ( value == true )
	{
		switch ( effect )
		{
			case EFF_MESSY:
				if ( myStats->mask && myStats->mask->type == MASK_HAZARD_GOGGLES )
				{
					bool shapeshifted = false;
					if ( behavior == &actPlayer && myStats->type != HUMAN )
					{
						if ( effectShapeshift != NOTHING )
						{
							shapeshifted = true;
						}
					}
					if ( !shapeshifted )
					{
						return false;
					}
				}
				break;
			case EFF_GREASY:
				if ( myStats->type == GOATMAN )
				{
					return false;
				}
				break;
			case EFF_CONFUSED:
				if ( myStats->type == LICH || myStats->type == DEVIL
					|| myStats->type == LICH_FIRE || myStats->type == LICH_ICE
					|| myStats->type == MINOTAUR || myStats->type == MIMIC )
				{
					return false;
				}
				break;
			case EFF_ASLEEP:
			case EFF_PARALYZED:
			case EFF_PACIFY:
			case EFF_KNOCKBACK:
			case EFF_BLIND:
			case EFF_WEBBED:
				if ( (myStats->type >= LICH && myStats->type < KOBOLD)
					|| myStats->type == COCKATRICE || myStats->type == LICH_FIRE || myStats->type == LICH_ICE )
				{
					if ( !(effect == EFF_PACIFY && myStats->type == SHOPKEEPER) &&
						!(effect == EFF_KNOCKBACK && myStats->type == COCKATRICE) &&
						!(effect == EFF_WEBBED && myStats->type == COCKATRICE) &&
						!(effect == EFF_BLIND && myStats->type == COCKATRICE) &&
						!(effect == EFF_BLIND && myStats->type == SHOPKEEPER) )
					{
						return false;
					}
				}
				break;
			case EFF_DISORIENTED:
			case EFF_ROOTED:
				if ( myStats->type == LICH || myStats->type == DEVIL
					|| myStats->type == LICH_FIRE || myStats->type == LICH_ICE
					|| myStats->type == SHADOW || myStats->type == SHOPKEEPER )
				{
					return false;
				}
				break;
			case EFF_FEAR:
				if ( myStats->type == LICH || myStats->type == DEVIL 
					|| myStats->type == LICH_FIRE || myStats->type == LICH_ICE
					|| myStats->type == SHADOW )
				{
					return false;
				}
				break;
			case EFF_POLYMORPH:
				//if ( myStats->EFFECTS[EFF_POLYMORPH] || effectPolymorph != 0 )
				//{
				//	return false;
				//}
				break;
			case EFF_BLEEDING:
				if ( gibtype[(int)myStats->type] == 0 || gibtype[(int)myStats->type] == 5 )
				{
					return false;
				}
				break;
			default:
				break;
		}
	}
	else
	{
		if ( effect == EFF_MIMIC_LOCKED )
		{
			myStats->monsterMimicLockedBy = 0;
		}
	}
	myStats->EFFECTS[effect] = value;
	myStats->EFFECTS_TIMERS[effect] = duration;

	int player = -1;
	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		if ( players[i] && players[i]->entity == this )
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
    clientsHaveItsStats = true;
}

void Entity::monsterAcquireAttackTarget(const Entity& target, Sint32 state, bool monsterWasHit)
{
	Stat* myStats = getStats();
	if ( !myStats )
	{
		return;
	}

	bool hadOldTarget = (uidToEntity(monsterTarget) != nullptr);
	Sint32 oldMonsterState = monsterState;

	if ( target.getRace() == GYROBOT || target.isInertMimic() || target.isUntargetableBat() )
	{
		return;
	}
	else if ( myStats->type == GYROBOT )
	{
		if ( state == MONSTER_STATE_ATTACK )
		{
			return;
		}
		else
		{
			if ( target.behavior == &actMonster )
			{
				return;
			}
		}
	}
	else if ( myStats->type == MIMIC && isInertMimic() )
	{
		return;
	}
	else if ( myStats->type == BAT_SMALL && monsterSpecialState == BAT_REST )
	{
		return;
	}
	else if ( monsterIsImmobileTurret(this, myStats) )
	{
		if ( monsterAllyIndex >= 0 && target.behavior == &actPlayer )
		{
			return;
		}
		if ( monsterIsImmobileTurret(nullptr, target.getStats()) && state == MONSTER_STATE_PATH )
		{
			return;
		}
		if ( monsterState == MONSTER_STATE_WAIT )
		{
			if ( myStats->LVL >= 10 && monsterHitTime < HITRATE )
			{
				monsterHitTime = HITRATE;
			}
		}
	}

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
	else if ( myStats->type == INCUBUS && !strncmp(myStats->name, "inner demon", strlen("inner demon")) )
	{
		if ( monsterState == MONSTER_STATE_WAIT )
		{
			return;
		}
	}
	else if ( myStats->type == BUGBEAR )
	{
		Stat* targetStats = target.getStats();
		if ( targetStats && targetStats->type == BUGBEAR )
		{
			if ( (targetStats && targetStats->leader_uid == getUID()) || target.parent == getUID() )
			{
				return;
			}
		}
	}

	if ( target.getRace() == INCUBUS )
	{
		Stat* targetStats = target.getStats();
		if ( targetStats && !strncmp(targetStats->name, "inner demon", strlen("inner demon")) )
		{
			Entity* illusionTauntingThisEntity = uidToEntity(static_cast<Uint32>(target.monsterIllusionTauntingThisUid));
			if ( illusionTauntingThisEntity != this )
			{
				return;
			}
		}
	}

	if ( myStats->EFFECTS[EFF_DISORIENTED] )
	{
		return;
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
					if ( myStats->weapon && itemCategory(myStats->weapon) == MAGICSTAFF )
					{
						monsterHitTime = HITRATE - 6; // 120 ms reaction time
					}
					else
					{
						monsterHitTime = 2 * HITRATE - 2;
					}
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

	if ( (myStats->type == SHOPKEEPER || myStats->type == HUMAN) && monsterTarget != target.getUID() )
	{
		Stat* targetStats = target.getStats();
		if ( targetStats )
		{
			if ( myStats->type == SHOPKEEPER )
			{
				for (int c = 0; c < MAXPLAYERS; ++c)
				{
					players[c]->worldUI.worldTooltipDialogue.createDialogueTooltip(getUID(),
						Player::WorldUI_t::WorldTooltipDialogue_t::DIALOGUE_ATTACK, Language::get(3243),
						Language::get(4217 + local_rng.uniform(0, 16)), getMonsterLocalizedName(targetStats->type).c_str());
				}
				if (target.behavior == &actPlayer && targetStats->type != HUMAN && targetStats->type != AUTOMATON )
				{
					steamAchievementClient(target.skill[2], "BARONY_ACH_RIGHT_TO_REFUSE");
				}
			}
			else
			{
				if ( monsterAllyIndex < 0 || (monsterAllyIndex >= 0 && local_rng.getU8() % 8 == 0) )
				{
					for (int c = 0; c < MAXPLAYERS; ++c)
					{
						if (local_rng.getU8() % 2) {
							players[c]->worldUI.worldTooltipDialogue.createDialogueTooltip(getUID(),
								Player::WorldUI_t::WorldTooltipDialogue_t::DIALOGUE_ATTACK, Language::get(516 + local_rng.uniform(0, 1)),
								Language::get(4234 + local_rng.uniform(0, 16)), getMonsterLocalizedName(targetStats->type).c_str());
						} else {
							players[c]->worldUI.worldTooltipDialogue.createDialogueTooltip(getUID(),
								Player::WorldUI_t::WorldTooltipDialogue_t::DIALOGUE_ATTACK, Language::get(518 + local_rng.uniform(0, 1)),
								Language::get(4217 + local_rng.uniform(0, 16)), getMonsterLocalizedName(targetStats->type).c_str());
						}
					}
				}
			}

			if ( myStats->type == SHOPKEEPER && target.behavior == &actPlayer )
			{
				if ( oldMonsterState == MONSTER_STATE_TALK && monsterState != MONSTER_STATE_TALK )
				{
					for ( int i = 0; i < MAXPLAYERS; ++i )
					{
						if ( players[i]->isLocalPlayer() && shopkeeper[i] == getUID() )
						{
							players[i]->closeAllGUIs(CLOSEGUI_ENABLE_SHOOTMODE, CLOSEGUI_CLOSE_ALL);
						}
						else if ( i > 0 && !client_disconnected[i] && multiplayer == SERVER && !players[i]->isLocalPlayer() )
						{
							// inform client of abandonment
							strcpy((char*)net_packet->data, "SHPC");
							SDLNet_Write32(getUID(), &net_packet->data[4]);
							net_packet->address.host = net_clients[i - 1].host;
							net_packet->address.port = net_clients[i - 1].port;
							net_packet->len = 8;
							sendPacketSafe(net_sock, -1, net_packet, i - 1);
						}
					}
				}
			}
		}
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
			case SHOPKEEPER:
				if ( myStats->MISC_FLAGS[STAT_FLAG_MYSTERIOUS_SHOPKEEP] > 0 )
				{
					monsterAddNearbyItemToInventory(myStats, 16, 99);
				}
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

bool Entity::monsterAddNearbyItemToInventory(Stat* myStats, int rangeToFind, int maxInventoryItems, Entity* forcePickupItem)
{
	//TODO: Any networking/multiplayer needs?
	if ( !myStats )
	{
		return false; //Can't continue without these.
	}

	if ( list_Size(&myStats->inventory) >= maxInventoryItems + 1 )
	{
		return false;
	}

	list_t* itemsList = nullptr;
	//X and Y in terms of tiles.
	if ( forcePickupItem != nullptr && forcePickupItem->behavior == &actItem )
	{
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			if ( FollowerMenu[i].followerToCommand == this )
			{
				if ( !FollowerMenu[i].allowedInteractItems(myStats->type) )
				{
					return false;
				}
				break;
			}
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
	bool pickedUpItemReturnValue = false;

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
						if ( item != nullptr )
						{
							free(item);
						}
						continue;
					}
				}
				if ( entity->itemDelayMonsterPickingUp > 0 && entity->ticks < entity->itemDelayMonsterPickingUp )
				{
					// dropped from a disarm skill, don't pick up item until timer is up.
					if ( item != nullptr )
					{
						free(item);
					}
					continue;
				}

				if ( myStats->type == SHOPKEEPER && myStats->MISC_FLAGS[STAT_FLAG_MYSTERIOUS_SHOPKEEP] > 0 )
				{
					// pickup the item always.
					Entity* owner = uidToEntity(item->ownerUid);
					if ( owner && owner->behavior == &actPlayer )
					{
						for ( int c = 0; c < MAXPLAYERS; ++c )
						{
							switch ( item->type )
							{
								case ARTIFACT_ORB_GREEN:
									//messagePlayer(owner->skill[2], MESSAGE_WORLD, Language::get(3888), myStats->name);
									players[c]->worldUI.worldTooltipDialogue.createDialogueTooltip(getUID(),
										Player::WorldUI_t::WorldTooltipDialogue_t::DIALOGUE_BROADCAST, Language::get(3888));
									if ( c == owner->skill[2] )
									{
										Compendium_t::Events_t::eventUpdateMonster(owner->skill[2], Compendium_t::CPDM_MERCHANT_ORBS, this, 1);
									}
									break;
								case ARTIFACT_ORB_BLUE:
									//messagePlayer(owner->skill[2], MESSAGE_WORLD, Language::get(3889), myStats->name);
									players[c]->worldUI.worldTooltipDialogue.createDialogueTooltip(getUID(),
										Player::WorldUI_t::WorldTooltipDialogue_t::DIALOGUE_BROADCAST, Language::get(3889));
									if ( c == owner->skill[2] )
									{
										Compendium_t::Events_t::eventUpdateMonster(owner->skill[2], Compendium_t::CPDM_MERCHANT_ORBS, this, 1);
									}
									break;
								case ARTIFACT_ORB_RED:
									//messagePlayer(owner->skill[2], MESSAGE_WORLD, Language::get(3890), myStats->name);
									players[c]->worldUI.worldTooltipDialogue.createDialogueTooltip(getUID(),
										Player::WorldUI_t::WorldTooltipDialogue_t::DIALOGUE_BROADCAST, Language::get(3890));
									if ( c == owner->skill[2] )
									{
										Compendium_t::Events_t::eventUpdateMonster(owner->skill[2], Compendium_t::CPDM_MERCHANT_ORBS, this, 1);
									}
									break;
								default:
									break;
							}
						}
					}
					playSoundEntity(this, 35 + local_rng.rand() % 3, 64);
					addItemToMonsterInventory(item);
					item = nullptr;
					list_RemoveNode(entity->mynode);
					pickedUpItemReturnValue = true;
				}
				else if ( myStats->type == SLIME )
				{
					if ( item->identified )
					{
						messagePlayer(monsterAllyIndex, MESSAGE_WORLD, Language::get(3145), items[item->type].getIdentifiedName());
					}
					else
					{
						messagePlayer(monsterAllyIndex, MESSAGE_WORLD, Language::get(3145), items[item->type].getUnidentifiedName());
					}
					list_RemoveNode(entity->mynode); // slimes eat the item up.
					pickedUpItemReturnValue = true;
				}
				else if ( shouldWield )
				{
					if ( (*shouldWield) && (*shouldWield)->beatitude < 0 && myStats->type != AUTOMATON )
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
							if ( monsterAllyIndex >= 0 && monsterAllyIndex < MAXPLAYERS )
							{
								if ( players[monsterAllyIndex] && players[monsterAllyIndex]->entity )
								{
									dropped->itemOriginalOwner = players[monsterAllyIndex]->entity->getUID();
								}
							}
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
							playSoundEntity(this, 33 + local_rng.rand() % 2, 64);
						}
						else if ( itemCategory(item) == WEAPON || itemCategory(item) == THROWN )
						{
							playSoundEntity(this, 40 + local_rng.rand() % 4, 64);
						}
						else if ( itemCategory(item) == ARMOR )
						{
							playSoundEntity(this, 44 + local_rng.rand() % 3, 64);
						}
						else if ( item->type == TOOL_TORCH || item->type == TOOL_LANTERN || item->type == TOOL_CRYSTALSHARD )
						{
							playSoundEntity(this, 134, 64);
						}
					}

					(*shouldWield) = item;
					item = nullptr;
					list_RemoveNode(entity->mynode);
					pickedUpItemReturnValue = true;
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
					pickedUpItemReturnValue = true;
				}
				else if ( list_Size(&myStats->inventory) < maxInventoryItems )
				{
					bool addItem = true;
					if ( myStats->type == GYROBOT && list_Size(&myStats->inventory) >= 1 )
					{
						node_t* inv = myStats->inventory.first;
						Item* toStack = (Item*)inv->element;
						if ( toStack )
						{
							if ( toStack->type >= TOOL_BOMB && toStack->type <= TOOL_TELEPORT_BOMB )
							{
								if ( !itemCompare(toStack, item, false) )
								{
									// stack the items.
									toStack->count += item->count;
									item = nullptr;
									list_RemoveNode(entity->mynode);
									pickedUpItemReturnValue = true;
									addItem = false;
								}
							}
						}
					}

					if ( addItem )
					{
						addItemToMonsterInventory(item);
						item = nullptr;
						list_RemoveNode(entity->mynode);
						pickedUpItemReturnValue = true;
					}
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

	return pickedUpItemReturnValue;
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

	if ( myStats->type == SHOPKEEPER )
	{
		// sort items into slots
		std::vector<std::pair<int, Item*>> priceAndItems;
		for ( node_t* node = myStats->inventory.first; node != nullptr; node = node->next )
		{
			Item* item = (Item*)node->element;
			if ( !item ) { continue; }

			if ( !item->itemSpecialShopConsumable )
			{
				priceAndItems.push_back(std::make_pair(item->buyValue(clientnum), item));
			}
		}

		std::sort(priceAndItems.begin(), priceAndItems.end(), [](std::pair<int, Item*> lhs, std::pair<int, Item*> rhs) {
			return lhs.first > rhs.first;
		});

		int slotx = 0;
		int sloty = 0;
		for ( auto& v : priceAndItems )
		{
			Item* item = v.second;
			item->x = slotx;
			item->y = sloty;
			++slotx;
			if ( slotx >= Player::ShopGUI_t::MAX_SHOP_X )
			{
				slotx = 0;
				++sloty;
			}
		}
	}

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

	if ( !isRangedWeapon(itemToEquip) && isRangedWeapon(*(myStats->weapon)) && rangedWeaponUseQuiverOnAttack(myStats) )
	{
		// have ranged weapon and quiver, don't pickup non-ranged weapon.
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

	if ( myStats->type == GYROBOT && item.interactNPCUid == getUID() )
	{
		return true;
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
					if ( item.type == ARTIFACT_ORB_BLUE
						|| item.type == ARTIFACT_ORB_GREEN
						|| item.type == ARTIFACT_ORB_RED
						|| item.type == ARTIFACT_ORB_PURPLE )
					{
						return false;
					}
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
			if ( item.type == ARTIFACT_ORB_BLUE
				|| item.type == ARTIFACT_ORB_GREEN
				|| item.type == ARTIFACT_ORB_RED
				|| item.type == ARTIFACT_ORB_PURPLE )
			{
				return false;
			}
			return true; // noms on all items.
			break;
		case SHOPKEEPER:
			if ( myStats->MISC_FLAGS[STAT_FLAG_MYSTERIOUS_SHOPKEEP] > 0 )
			{
				if ( item.type == ARTIFACT_ORB_BLUE
					|| item.type == ARTIFACT_ORB_GREEN
					|| item.type == ARTIFACT_ORB_RED )
				{
					return true;
				}
			}
			return false;
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
				node_t* weaponNode = itemNodeInInventory(myStats, -1, WEAPON);
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
			if ( itemTypeIsQuiver(item.type) )
			{
				return (shouldEquip = shouldMonsterEquipThisArmor(item));
			}
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
		case TYPE_OFFHAND:
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
	int race = getMonsterTypeFromSprite();
	if ( race == SENTRYBOT || race == SPELLBOT )
	{
		Stat* myStats = getStats();
		int ratio = 64;
		if ( myStats )
		{
			if ( myStats->LVL >= 15 )
			{
				ratio = 2;
			}
			else if ( myStats->LVL >= 10 )
			{
				ratio = 4;
			}
			else if ( myStats->LVL >= 5 )
			{
				ratio = 16;
			}
			else if ( myStats->LVL >= 3 )
			{
				ratio = 64;
			}
		}
		yaw -= dir / ratio;
	}
	else if ( race == DUMMYBOT )
	{
		yaw -= dir / 4;
	}
	else if ( race == MIMIC )
	{
		yaw -= dir / 4;
	}
	else
	{
		yaw -= dir / 2;
	}
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

	if ( currentBest && itemIsThrowableTinkerTool(currentBest) )
	{
		currentBest = nullptr;
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
	if ( hitstats.type == SHADOW || hitstats.type == LICH || hitstats.type == LICH_FIRE || hitstats.type == LICH_ICE )
	{
		return; //Shadows' armor and shields don't break.
	}

	if ( hitstats.type == SKELETON && behavior == &actMonster && monsterAllySummonRank > 0 )
	{
		return; // conjured skeleton armor doesn't break.
	}

	if ( armor.type == ARTIFACT_BOOTS
		|| armor.type == ARTIFACT_HELM
		|| armor.type == ARTIFACT_CLOAK
		|| armor.type == ARTIFACT_GLOVES
		|| armor.type == ARTIFACT_BREASTPIECE
		|| armor.type == MASK_ARTIFACT_VISOR )
	{
		return;
	}

	if ( itemTypeIsQuiver(armor.type) )
	{
		// quivers don't break.
		return;
	}

	int playerhit = -1;

	if ( this->behavior == &actPlayer )
	{
		playerhit = this->skill[2];
	}

	if ( armor.type == TOOL_TORCH && armor.count > 1 && playerhit >= 0 && &armor == stats[playerhit]->shield )
	{
		std::string itemName = armor.getName();
		ItemType itemType = armor.type;
		Status itemStatus = armor.status;

		playSoundEntity(this, 76, 64);
		messagePlayer(playerhit, MESSAGE_EQUIPMENT, Language::get(682), armor.getName()); // torch is destroyed

		int qty = std::max(0, armor.count - 1);
		Item* item = stats[playerhit]->shield;
		consumeItem(item, playerhit);
		if ( qty > 0 && item )
		{
			messagePlayer(playerhit, MESSAGE_EQUIPMENT, Language::get(4101), itemName.c_str()); // you reignite another torch
			playSoundEntity(this, 134, 64); // ignite
			if ( playerhit >= 0 && players[playerhit]->isLocalPlayer() )
			{
				players[playerhit]->hud.shieldSwitch = true;
			}
		}
		if ( !item )
		{
			stats[playerhit]->shield = nullptr;
		}

		if ( multiplayer == SERVER && playerhit > 0 && !players[playerhit]->isLocalPlayer() )
		{
			strcpy((char*)net_packet->data, "TORC");
			SDLNet_Write16((Sint16)itemType, &net_packet->data[4]);
			net_packet->data[6] = itemStatus;
			net_packet->data[7] = qty;
			net_packet->address.host = net_clients[playerhit - 1].host;
			net_packet->address.port = net_clients[playerhit - 1].port;
			net_packet->len = 8;
			sendPacketSafe(net_sock, -1, net_packet, playerhit - 1);
		}
		if ( hitstats.defending )
		{
			Compendium_t::Events_t::eventUpdate(playerhit, Compendium_t::CPDM_BROKEN_BY_BLOCKING, armor.type, 1);
		}
		Compendium_t::Events_t::eventUpdate(playerhit, Compendium_t::CPDM_BROKEN, armor.type, 1);
		return;
	}

	if ( (playerhit >= 0 && players[playerhit]->isLocalPlayer()) || playerhit < 0 )
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
			messagePlayer(playerhit, MESSAGE_EQUIPMENT, Language::get(2350), armor.getName());
		}
		else
		{
			messagePlayer(playerhit, MESSAGE_EQUIPMENT, Language::get(681), armor.getName());
		}
		if ( playerhit >= 0 )
		{
			Compendium_t::Events_t::eventUpdate(playerhit, Compendium_t::CPDM_DEGRADED, armor.type, 1);
		}
	}
	else
	{
		if ( armor.type == TOOL_CRYSTALSHARD )
		{
			playSoundEntity(this, 162, 64);
			messagePlayer(playerhit, MESSAGE_EQUIPMENT, Language::get(2351), armor.getName());
		}
		else if ( itemCategory(&armor) == SPELLBOOK )
		{
			playSoundEntity(this, 414, 64);
			messagePlayer(playerhit, MESSAGE_EQUIPMENT, Language::get(3459), armor.getName());
		}
		else
		{
			playSoundEntity(this, 76, 64);
			messagePlayer(playerhit, MESSAGE_EQUIPMENT, Language::get(682), armor.getName());
		}
		if ( playerhit >= 0 )
		{
			if ( hitstats.defending )
			{
				Compendium_t::Events_t::eventUpdate(playerhit, Compendium_t::CPDM_DEGRADED, armor.type, 1);
				Compendium_t::Events_t::eventUpdate(playerhit, Compendium_t::CPDM_BROKEN_BY_BLOCKING, armor.type, 1);
			}
			Compendium_t::Events_t::eventUpdate(playerhit, Compendium_t::CPDM_BROKEN, armor.type, 1);
		}
	}
	if ( playerhit > 0 && multiplayer == SERVER && !players[playerhit]->isLocalPlayer() )
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
	// gnomes, spiders, humans (50%)
	// kobolds, scarabs, suc/incubi, goatmen, rats

	// excluded golems, shadows, cockatrice, skeletons, demons, imps
	// scorpions, slimes, ghouls, vampires, shopkeeps

	// retreating monsters will not try path when losing sight of target

	if ( myStats.EFFECTS[EFF_PACIFY] || myStats.EFFECTS[EFF_FEAR] )
	{
		return true;
	}
	if ( myStats.EFFECTS[EFF_KNOCKBACK] )
	{
		return true;
	}
	if ( (myStats.EFFECTS[EFF_DASH] || (myStats.weapon && myStats.weapon->type == SPELLBOOK_DASH)) && behavior == &actMonster )
	{
		return false;
	}
	if ( myStats.EFFECTS[EFF_ROOTED] )
	{
		return false;
	}
	if ( myStats.type == VAMPIRE )
	{
		return false;
	}
	else if ( myStats.type == SHADOW )
	{
		return false;
	}
	else if ( myStats.type == SHOPKEEPER )
	{
		return false;
	}
	else if ( myStats.type == MIMIC )
	{
		return false;
	}
	else if ( myStats.type == GNOME )
	{
		if ( myStats.getAttribute("gnome_type").find("_melee") != std::string::npos )
		{
			if ( myStats.leader_uid != 0 )
			{
				return false;
			}
		}
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
	else if ( monsterIsImmobileTurret(this, &myStats) )
	{
		return false;
	}
	else if ( myStats.type == SPIDER )
	{
		if ( monsterTarget != 0 )
		{
			if ( Entity* target = uidToEntity(monsterTarget) )
			{
				if ( Stat* targetStats = target->getStats() )
				{
					if ( targetStats->EFFECTS[EFF_WEBBED] )
					{
						return false;
					}
				}
			}
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
	if ( myStats.type == INCUBUS && !strncmp(myStats.name, "inner demon", strlen("inner demon")) )
	{
		return false;
	}
	if ( monsterTarget != 0 && myStats.monsterDemonHasBeenExorcised != 0 )
	{
		Entity* target = uidToEntity(monsterTarget);
		if ( target )
		{
			Stat* targetStats = target->getStats();
			if ( targetStats && targetStats->type == INCUBUS && !strncmp(targetStats->name, "inner demon", strlen("inner demon")) )
			{
				return false;
			}
		}
	}

	Entity* leader = monsterAllyGetPlayerLeader();
	if ( leader )
	{
		// do not retreat for brave leader!
		return false;
	}

	if ( myStats.MAXHP >= 100 )
	{
		if ( myStats.HP <= myStats.MAXHP / 8 && this->getCHR() >= -2 )
		{
			return true;
		}
	}
	else if ( myStats.HP <= myStats.MAXHP / 4 && this->getCHR() >= -2 )
	{
		return true;
	}

	return false;
}

bool Entity::backupWithRangedWeapon(Stat& myStats, int dist, int hasrangedweapon)
{
	int distanceLimit = 100;
	if ( hasrangedweapon && myStats.weapon )
	{
		if ( distanceLimit >= getMonsterEffectiveDistanceOfRangedWeapon(myStats.weapon) )
		{
			distanceLimit = getMonsterEffectiveDistanceOfRangedWeapon(myStats.weapon) - 20;
		}
	}
	if ( dist >= distanceLimit || !hasrangedweapon )
	{
		return false;
	}

	if ( (myStats.EFFECTS[EFF_DASH] || (myStats.weapon && myStats.weapon->type == SPELLBOOK_DASH)) && behavior == &actMonster )
	{
		return false;
	}
	if ( myStats.EFFECTS[EFF_ROOTED] )
	{
		return false;
	}
	if ( myStats.type == INSECTOID && monsterSpecialState > 0 )
	{
		return false;
	}
	if ( myStats.type == SPIDER && monsterSpecialState > 0 )
	{
		return false;
	}
	if ( monsterIsImmobileTurret(this, &myStats) )
	{
		return false;
	}
	if ( myStats.type == VAMPIRE && (monsterSpecialState > 0 || MonsterData_t::nameMatchesSpecialNPCName(myStats, "bram kindly")) )
	{
		return false;
	}
	Entity* leader = monsterAllyGetPlayerLeader();
	if ( leader )
	{
		if ( monsterTarget != 0 )
		{
			if ( Entity* target = uidToEntity(monsterTarget) )
			{
				if ( target->behavior == &actMonster && target->monsterTarget == getUID() ) // my target is attacking me
				{
					if ( dist < TOUCHRANGE * 2 )
					{
						return true;
					}
				}
			}
			return false;
		}
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
		//messagePlayer(clientnum, "DEBUG: Monster has spell %s.", spell->getSpellName());
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
			//messagePlayer(clientnum, "DEBUG: Monster HAS spell %s.", spell->getSpellName());
			return true;
		}
	}

	return false;
}

void Entity::playerStatIncrease(int playerClass, int chosenStats[3])
{
	std::vector<unsigned int> statWeights = classStatGrowth[playerClass];

	// debug to print which vector values are being used.
	//for ( std::vector<int>::const_iterator i = statWeights.begin(); i != statWeights.end(); ++i )
	//{
	//	messagePlayer(0, "%2d, ", *i);
	//}
	if ( behavior == &actPlayer && playerClass == CLASS_SHAMAN && stats[skill[2]] )
	{
		if ( stats[skill[2]]->type == RAT )
		{
			//	            STR	DEX	CON	INT	PER	CHR
			statWeights = { 1,	6,	1,	3,	1,	1 };
		}
		else if ( stats[skill[2]]->type == SPIDER )
		{
			//	            STR	DEX	CON	INT	PER	CHR
			statWeights = { 1,	1,	3,	1,	6,	1 };
		}
		else if ( stats[skill[2]]->type == TROLL )
		{
			//	            STR	DEX	CON	INT	PER	CHR
			statWeights = { 6,	1,	3,	1,	1,	1 };
		}
		else if ( stats[skill[2]]->type == CREATURE_IMP )
		{
			//	            STR	DEX	CON	INT	PER	CHR
			statWeights = { 1,	3,	1,	6,	1,	1 };
		}
	}

	bool forceInt = false;
	if ( Stat* stat = getStats() )
	{
		if ( stat->helmet && stat->helmet->type == HAT_CIRCLET_WISDOM )
		{
			if ( stat->helmet->beatitude >= 0 || shouldInvertEquipmentBeatitude(stat) )
			{
				// int bonus last stat
				forceInt = true;
			}
			else
			{
				// int never picked
				statWeights[STAT_INT] = 0;
			}
		}
	}
	chosenStats[0] = local_rng.rand() % 6; // get first stat randomly.
	statWeights[chosenStats[0]] = 0; // remove the chance of the local stat vector.

	chosenStats[1] = local_rng.discrete(statWeights.data(), statWeights.size()); // get second stat.
	statWeights[chosenStats[1]] = 0; // remove the chance in the local stat vector.

	if ( forceInt && chosenStats[0] != STAT_INT && chosenStats[1] != STAT_INT )
	{
		chosenStats[2] = STAT_INT;
	}
	else
	{
		chosenStats[2] = local_rng.discrete(statWeights.data(), statWeights.size()); // get third stat.
	}

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

	spellbook = spellbookNodeInInventory(myStats, spellbooks[local_rng.rand()%spellbooks.size()]); //Choose a random spell and return it.
	if (!spellbook )
	{
		//messagePlayer(clientnum, "[DEBUG:Entity::chooseAttackSpellbookFromInventory()] Error: Failed to choose a spellbook!");
	}
	return spellbook;
}

int Entity::getManaringFromEffects(Entity* my, Stat& myStats)
{
	int manaring = 0;
	if ( myStats.EFFECTS[EFF_MP_REGEN] && myStats.type != AUTOMATON )
	{
		manaring += 2;
	}
	return manaring;
}

int Entity::getManaringFromEquipment(Entity* my, Stat& myStats, bool isPlayer)
{
	bool shapeshifted = false;
	if ( isPlayer && myStats.type != HUMAN )
	{
		if ( my && my->effectShapeshift != NOTHING )
		{
			shapeshifted = true;
		}
	}

	int manaring = 0;
	bool cursedItemIsBuff = false;
	if ( isPlayer )
	{
		cursedItemIsBuff = shouldInvertEquipmentBeatitude(&myStats);
	}
	if ( myStats.breastplate != nullptr )
	{
		if ( myStats.breastplate->type == VAMPIRE_DOUBLET )
		{
			if ( myStats.breastplate->beatitude >= 0 || cursedItemIsBuff )
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
			if ( myStats.cloak->beatitude >= 0 || cursedItemIsBuff )
			{
				manaring++;
			}
			else
			{
				manaring--;
			}
		}
	}
	if ( myStats.mask != nullptr )
	{
		if ( myStats.mask->type == MASK_SHAMAN && shapeshifted )
		{
			if ( myStats.mask->beatitude >= 0 || cursedItemIsBuff )
			{
				manaring++;
			}
			else
			{
				manaring--;
			}
		}
	}
	return manaring;
}

int Entity::getManaRegenInterval(Entity* my, Stat& myStats, bool isPlayer)
{
	int regenTime = getBaseManaRegen(my, myStats);
	int manaring = 0;
	if ( isPlayer && myStats.type != HUMAN )
	{
		if ( myStats.type == SKELETON )
		{
			manaring = -1; // 0.25x regen speed.
		}
	}

	int bonusManaring = 0;
	bonusManaring += Entity::getManaringFromEquipment(my, myStats, true);
	bonusManaring += Entity::getManaringFromEffects(my, myStats);
	manaring += bonusManaring;

	if ( my && bonusManaring >= 2 && ::ticks % TICKS_PER_SECOND == 0 && isPlayer )
	{
		bool oldRegen = myStats.EFFECTS[EFF_MP_REGEN];
		myStats.EFFECTS[EFF_MP_REGEN] = false;
		int bonusManaringNoRegen = Entity::getManaringFromEquipment(my, myStats, true) + Entity::getManaringFromEffects(my, myStats);
		if ( bonusManaringNoRegen >= 2 )
		{
			steamAchievementEntity(my, "BARONY_ACH_ARCANE_LINK");
		}
		myStats.EFFECTS[EFF_MP_REGEN] = oldRegen;
	}

	if ( manaring > 3 )
	{
		manaring = 3;
	}

	if ( isPlayer && myStats.type == AUTOMATON && myStats.HUNGER <= 300 )
	{
		float floatRegenTime = (60 * regenTime) / (std::max(myStats.MAXMP, 1));
		if ( manaring > 0 )
		{
			return floatRegenTime * (manaring * 2); // lose 1 MP each 12 base seconds - good!
		}
		else if ( manaring < 0 )
		{
			return floatRegenTime / (abs(manaring) * 2); // lose 1 MP each 3 base seconds - bad!
		}
		else if ( manaring == 0 )
		{
			return floatRegenTime;
		}
	}
	else if ( isPlayer && myStats.playerRace == RACE_INSECTOID && myStats.stat_appearance == 0 )
	{
		if ( !(svFlags & SV_FLAG_HUNGER) )
		{
			return -1;
		}

		// how many hunger ticks in seconds from max of 1000.
		float floatRegenTime = (1000.f * (Entity::getHungerTickRate(&myStats, isPlayer, true)) / static_cast<float>(TICKS_PER_SECOND)); 

		floatRegenTime /= (std::max(myStats.MAXMP, 1)); // time for 1 mana in seconds
		floatRegenTime *= TICKS_PER_SECOND; // game ticks for 1 mana
		return floatRegenTime;
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

int Entity::getHealringFromEffects(Entity* my, Stat& myStats)
{
	double healring = 0;
	if ( myStats.EFFECTS[EFF_HP_REGEN] )
	{
		if ( my && my->monsterAllyGetPlayerLeader() && my->monsterAllySpecial == ALLY_SPECIAL_CMD_REST && myStats.EFFECTS[EFF_ASLEEP] )
		{
			healring += 1;
		}
		else
		{
			healring += 2;
		}
	}
	if ( myStats.EFFECTS[EFF_TROLLS_BLOOD] )
	{
		healring += 1;
	}
	if ( my )
	{
		if ( int bonusFollowerRegen = my->getFollowerBonusHPRegen() )
		{
			if ( bonusFollowerRegen < 0 )
			{
				healring += bonusFollowerRegen;
			}
		}
	}
	return (int)healring;
}

int Entity::getHealringFromEquipment(Entity* my, Stat& myStats, bool isPlayer)
{
	double healring = 0;
	bool cursedItemIsBuff = false;
	if ( isPlayer )
	{
		cursedItemIsBuff = shouldInvertEquipmentBeatitude(&myStats);
	}

	if ( myStats.ring != nullptr )
	{
		if ( myStats.ring->type == RING_REGENERATION )
		{
			if ( myStats.ring->beatitude >= 0 || cursedItemIsBuff )
			{
				healring++;
				if ( cursedItemIsBuff )
				{
					healring += std::min(static_cast<int>(abs(myStats.ring->beatitude)), 1);
				}
				else
				{
					healring += std::min(static_cast<int>(myStats.ring->beatitude), 1);
				}
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
			if ( myStats.breastplate->beatitude >= 0 || cursedItemIsBuff )
			{
				healring++;
			}
			else
			{
				healring--;
			}
		}
	}
	return healring;
}

int Entity::getHealthRegenInterval(Entity* my, Stat& myStats, bool isPlayer)
{
	if ( myStats.EFFECTS[EFF_VAMPIRICAURA] )
	{
		if ( isPlayer && myStats.EFFECTS_TIMERS[EFF_VAMPIRICAURA] > 0 )
		{
			return -1;
		}
	}
	if ( myStats.HP <= 0 )
	{
		return -1;
	}

	if ( myStats.breastplate && myStats.breastplate->type == VAMPIRE_DOUBLET )
	{
		return -1;
	}

	if ( svFlags & SV_FLAG_HUNGER )
	{
		if ( isPlayer )
		{
			if ( myStats.HUNGER <= 0 )
			{
				bool doStarvation = true;
				if ( myStats.type == AUTOMATON )
				{
					if ( myStats.MP > 0 )
					{
						doStarvation = false;
					}
				}
				else if ( myStats.type == SKELETON )
				{
					doStarvation = false;
				}
				if ( doStarvation )
				{
					return -1;
				}
			}
		}
	}

	double healring = 0;
	double bonusHealring = 0.0;
	if ( myStats.type == SKELETON && isPlayer )
	{
		healring = -1;
	}
	if ( !(svFlags & SV_FLAG_HUNGER) && isPlayer )
	{
		bonusHealring += Entity::getHealringFromEquipment(my, myStats, isPlayer);
		bonusHealring += Entity::getHealringFromEffects(my, myStats);
		if ( bonusHealring < 0.01 && myStats.type != SKELETON )
		{
			return -1;
		}
	}
	else
	{
		bonusHealring += Entity::getHealringFromEquipment(my, myStats, isPlayer);
		bonusHealring += Entity::getHealringFromEffects(my, myStats);
	}
	healring += bonusHealring;

	if ( my && bonusHealring >= 2.0 && ::ticks % TICKS_PER_SECOND == 0 && isPlayer )
	{
		bool oldRegen = myStats.EFFECTS[EFF_HP_REGEN];
		myStats.EFFECTS[EFF_HP_REGEN] = false;
		int bonusHealringNoRegen = Entity::getHealringFromEquipment(my, myStats, isPlayer) + Entity::getHealringFromEffects(my, myStats);
		if ( bonusHealringNoRegen >= 2 )
		{
			steamAchievementEntity(my, "BARONY_ACH_TROLLS_BLOOD");
		}
		myStats.EFFECTS[EFF_HP_REGEN] = oldRegen;
	}
	
	if ( healring > 3 )
	{
		healring = 3;
	}

	if ( !strncmp(map.name, "Mages Guild", 11) && myStats.type == SHOPKEEPER )
	{
		healring = 25; // these guys like regenerating
	}

	if ( healring > 0 )
	{
		if ( !(svFlags & SV_FLAG_HUNGER) && isPlayer )
		{
			return (HEAL_TIME / (healring * 4)); // 1 HP each 12 sec base
		}
		else
		{
			return (HEAL_TIME / (healring * 6)); // 1 HP each 12 sec base
		}
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

int getBaseManaRegen(Entity* my, Stat& myStats)
{
	// reduced time from intelligence and spellcasting ability, 0-200 ticks of 300.
	int profMultiplier = (myStats.getModifiedProficiency(PRO_SPELLCASTING) / 20) + 1; // 1 to 6
	int statMultiplier = std::max(statGetINT(&myStats, my), 0); // get intelligence
	if ( myStats.type == AUTOMATON )
	{
		return MAGIC_REGEN_TIME;
	}

	int multipliedTotal = profMultiplier * statMultiplier;

	if ( myStats.weapon && myStats.weapon->type == ARTIFACT_MACE )
	{
		real_t amount = 0.0;
		getArtifactWeaponEffectChance(myStats.weapon->type, myStats, &amount);
		multipliedTotal += amount;
	}

	// unused - this is never hit by insectoid mana regen, old code
	//if ( behavior == &actPlayer && myStats.playerRace == RACE_INSECTOID && myStats.appearance == 0 )
	//{
	//	int base = MAGIC_REGEN_TIME / 3;
	//	if ( myStats.HUNGER < 50 )
	//	{
	//		base = MAGIC_REGEN_TIME * 3;
	//	}
	//	else if ( myStats.HUNGER < 250 )
	//	{
	//		base = MAGIC_REGEN_TIME;
	//	}
	//	return (base - static_cast<int>(std::min(multipliedTotal, 100))); // return 100-33 ticks, 2-0.67 seconds.
	//}

	return (MAGIC_REGEN_TIME - static_cast<int>(std::min(multipliedTotal, 200))); // return 300-100 ticks, 6-2 seconds.
}

void Entity::setRangedProjectileAttack(Entity& marksman, Stat& myStats, int optionalOverrideForArrowType)
{
	this->arrowSpeed = 7;
	this->arrowShotByWeapon = 0;
	this->arrowQuiverType = 0;

	// get arrow effects.
	if ( myStats.weapon )
	{
		this->arrowShotByWeapon = myStats.weapon->type;
		
		// no longer poisons!
		//if ( myStats.weapon->type == ARTIFACT_BOW )
		//{
		//	// poison arrow
		//	//this->arrowPoisonTime = 540;    // 9 seconds of poison
		//}

		if ( myStats.weapon->type != SLING )
		{
			// get armor pierce chance.
			int statChance = std::min(std::max(marksman.getPER() / 2, 0), 50); // 0 to 50 value.
			if ( myStats.weapon->type == HEAVY_CROSSBOW )
			{
				statChance += 50;
			}
			int chance = local_rng.rand() % 100;
			if ( chance < statChance )
			{
				this->arrowArmorPierce = 1; // pierce half of armor in damage calc.
			}
			else
			{
				this->arrowArmorPierce = 0;
			}
		}

		this->arrowDropOffEquipmentModifier = 0;
		if ( marksman.behavior == &actPlayer || marksman.behavior == &actMonster )
		{
			int dropOffModifier = 0;
			if ( myStats.helmet && myStats.helmet->type == HAT_BYCOCKET )
			{
				if ( myStats.helmet->beatitude >= 0 || shouldInvertEquipmentBeatitude(&myStats) )
				{
					dropOffModifier = std::min(3, 1 + abs(myStats.helmet->beatitude));
				}
				else
				{
					dropOffModifier = std::max(-4, -2 * abs(myStats.helmet->beatitude));
				}
			}
			arrowDropOffEquipmentModifier = std::min(7, std::max(-7, dropOffModifier));
		}

		if ( marksman.behavior == &actPlayer )
		{
			this->setArrowProjectileProperties(this->arrowShotByWeapon);
			this->arrowShotByParent = ARROW_SHOT_BY_PLAYER;
		}
		else if ( marksman.behavior == &actMonster )
		{
			this->arrowSpeed = 7;
			if ( myStats.type == SENTRYBOT )
			{
				this->arrowShotByParent = ARROW_SHOT_BY_TRAP;
			}
			else
			{
				this->setArrowProjectileProperties(this->arrowShotByWeapon);
				this->arrowShotByParent = ARROW_SHOT_BY_MONSTER;
			}
		}

		if ( multiplayer == SERVER )
		{
			Sint32 val = (1 << 31);
			val |= (Uint8)(17);
			val |= (((Uint16)(arrowShotByWeapon) & 0xFFF) << 8);
			val |= (arrowDropOffEquipmentModifier + 8) << 20;
			skill[2] = val;// -(1000 + arrowShotByWeapon); // invokes actArrow for clients.
		}
	}

	int attack = 0;

	if ( (myStats.shield && rangedWeaponUseQuiverOnAttack(&myStats)) || optionalOverrideForArrowType != WOODEN_SHIELD )
	{
		if ( optionalOverrideForArrowType != WOODEN_SHIELD )
		{
			this->arrowQuiverType = optionalOverrideForArrowType;
			if ( myStats.weapon )
			{
				ItemType oldType = myStats.weapon->type;
				myStats.weapon->type = static_cast<ItemType>(optionalOverrideForArrowType);
				attack += myStats.weapon->weaponGetAttack(&myStats);
				myStats.weapon->type = oldType;
			}
		}
		else
		{
			this->arrowQuiverType = myStats.shield->type;
			attack += myStats.shield->weaponGetAttack(&myStats);
		}
		switch ( arrowQuiverType )
		{
			case QUIVER_SILVER:
				sprite = 924;
				break;
			case QUIVER_PIERCE:
				arrowArmorPierce = 2;
				sprite = 925;
				break;
			case QUIVER_LIGHTWEIGHT:
				sprite = 926;
				break;
			case QUIVER_FIRE:
				sprite = 927;
				break;
			case QUIVER_KNOCKBACK:
				sprite = 928;
				break;
			case QUIVER_CRYSTAL:
				sprite = 929;
				break;
			case QUIVER_HUNTING:
				sprite = 930;
				break;
			default:
				break;
		}
	}

	// get arrow power.
	attack += marksman.getRangedAttack();
	real_t variance = 20;
	real_t baseSkillModifier = 50.0; // 40-60 base
	real_t skillModifier = baseSkillModifier - (variance / 2) + (myStats.getModifiedProficiency(PRO_RANGED) / 2.0);
	skillModifier += (local_rng.rand() % (1 + static_cast<int>(variance)));
	skillModifier /= 100.0;
	skillModifier = std::min(skillModifier, 1.0);
	attack = attack - static_cast<int>((1.0 - skillModifier) * attack);
	this->arrowPower = attack;
}

bool Entity::setArrowProjectileProperties(int weaponType)
{
	if ( weaponType == WOODEN_SHIELD )
	{
		return false;
	}
	if ( multiplayer == CLIENT && weaponType == TOOL_SENTRYBOT )
	{
		// hack for arrow traps.
		this->arrowSpeed = 7;
		this->vel_x = cos(this->yaw) * this->arrowSpeed;
		this->vel_y = sin(this->yaw) * this->arrowSpeed;
		return true;
	}

	if ( weaponType == CROSSBOW || weaponType == SLING || weaponType == HEAVY_CROSSBOW )
	{
		this->vel_z = -0.2;
		this->arrowSpeed = 6;
		this->pitch = -PI / 32;
		this->arrowFallSpeed = 0.1;
		this->arrowBoltDropOffRange = 5; // ticks before projectile starts falling.

		this->vel_x = cos(this->yaw) * this->arrowSpeed;
		this->vel_y = sin(this->yaw) * this->arrowSpeed;

		if ( this->arrowDropOffEquipmentModifier > 0 )
		{
			this->arrowBoltDropOffRange += 2 * (this->arrowDropOffEquipmentModifier);
		}
		else if ( this->arrowDropOffEquipmentModifier < 0 )
		{
			this->arrowBoltDropOffRange = 0;
		}
		return true;
	}
	else
	{
		this->vel_z = -0.6;
		this->arrowFallSpeed = 0.08;
		if ( weaponType == SHORTBOW || weaponType == COMPOUND_BOW || weaponType == ARTIFACT_BOW )
		{
			this->arrowSpeed = 7;
			this->vel_z = -0.6;
			this->arrowFallSpeed = 0.08;
		}
		else if ( weaponType == LONGBOW )
		{
			this->arrowSpeed = 8;
			this->vel_z = -0.4;
			this->arrowFallSpeed = 0.04;
		}
		this->pitch = -PI / 32;
		this->arrowBoltDropOffRange = 0;
		this->vel_x = cos(this->yaw) * this->arrowSpeed;
		this->vel_y = sin(this->yaw) * this->arrowSpeed;

		if ( this->arrowDropOffEquipmentModifier > 0 )
		{
			this->arrowFallSpeed *= 1.0 - (this->arrowDropOffEquipmentModifier) / 10.0;
		}
		else if ( this->arrowDropOffEquipmentModifier < 0 )
		{
			this->arrowFallSpeed *= 1.0 + (-this->arrowDropOffEquipmentModifier) / 10.0;
		}
		return true;
	}
	return false;
}

/* SetEntityOnFire
 * Attempts to set the Entity on fire. Entities that are not Burnable or are already on fire will return before any processing
 * Entities that do not have Stats (such as furniture) will return after setting the fire time and chance to stop at max
 * Entities with Stats will have their fire time (char_fire) and chance to stop being on fire (chanceToPutOutFire) reduced by their CON
 * Calculations for reductions is outlined in this function
 */
void Entity::SetEntityOnFire(Entity* sourceOfFire)
{
	// Check if the Entity can be set on fire
	if ( this->flags[BURNABLE] )
	{
		if ( this->behavior == &actPlayer )
		{
			Stat* myStats = this->getStats();
			if ( myStats )
			{
				if ( myStats->type == SKELETON )
				{
					return;
				}
				if ( myStats->type == AUTOMATON )
				{
					return;
				}
				if ( myStats->breastplate && myStats->breastplate->type == MACHINIST_APRON )
				{
					return;
				}
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

			if ( this->behavior == &actPlayer )
			{
				messagePlayerColor(this->skill[2], MESSAGE_COMBAT, makeColorRGB(255, 0, 0), Language::get(4324));
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

			if ( sourceOfFire && sourceOfFire->behavior == &actArrow )
			{
				if ( behavior == &actMonster )
				{
					// monsters shot with arrow burn less, harder for players.
					this->char_fire = std::min(this->char_fire, TICKS_TO_PROCESS_FIRE * 6);
				}
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

void messagePlayerMonsterEvent(int player, Uint32 color, Stat& monsterStats, const char* msgGeneric, const char* msgNamed, int detailType, Entity* optionalEntity)
{
	if ( player < 0 || player >= MAXPLAYERS )
	{
		return;
	}

	// If true, pretend the monster doesn't have a name and use the generic message "You hit the lesser skeleton!"
	bool namedMonsterAsGeneric = monsterNameIsGeneric(monsterStats);
	int monsterType = monsterStats.type;
	if ( optionalEntity != nullptr )
	{
		if ( optionalEntity->behavior == &actPlayer )
		{
			monsterType = optionalEntity->getMonsterTypeFromSprite();
		}
	}

	//char str[256] = { 0 };
	if ( !strcmp(monsterStats.name, "") )
	{
		// use generic racial name and grammar. "You hit the skeleton"
		switch (detailType)
		{
		case MSG_DESCRIPTION:
			messagePlayerColor(player, MESSAGE_INSPECTION, color, msgGeneric, getMonsterLocalizedName((Monster)monsterType).c_str());
			break;
		case MSG_COMBAT:
			messagePlayerColor(player, MESSAGE_COMBAT, color, msgGeneric, getMonsterLocalizedName((Monster)monsterType).c_str());
			break;
		case MSG_COMBAT_BASIC:
			messagePlayerColor(player, MESSAGE_COMBAT_BASIC, color, msgGeneric, getMonsterLocalizedName((Monster)monsterType).c_str());
			break;
		case MSG_OBITUARY:
			for ( int c = 0; c < MAXPLAYERS; ++c )
			{
				if ( client_disconnected[c] )
				{
					continue;
				}
				if ( c == player )
				{
					messagePlayerColor(c, MESSAGE_OBITUARY, color, msgNamed, getMonsterLocalizedName((Monster)monsterType).c_str(), monsterStats.obituary);
				}
				else if ( multiplayer != SINGLE )
				{
					messagePlayerColor(c, MESSAGE_OBITUARY, color, msgGeneric, stats[player]->name, getMonsterLocalizedName((Monster)monsterType).c_str(), monsterStats.obituary);
				}
			}
			break;
		default:
		case MSG_GENERIC:
			messagePlayerColor(player, MESSAGE_HINT, color, msgGeneric, getMonsterLocalizedName((Monster)monsterType).c_str());
			break;
		case MSG_ATTACKS:
			messagePlayerColor(player, MESSAGE_COMBAT_BASIC, color, msgGeneric,
				getMonsterLocalizedName((Monster)monsterType).c_str(),
				getMonsterLocalizedInjury((Monster)monsterType).c_str());
			break;
		case MSG_STEAL_WEAPON:
			if ( monsterStats.weapon )
			{
				messagePlayerColor(player, MESSAGE_COMBAT, color, msgGeneric, getMonsterLocalizedName((Monster)monsterType).c_str(), monsterStats.weapon->getName());
			}
			break;
		case MSG_TOOL_BOMB:
			if ( optionalEntity && optionalEntity->behavior == &actBomb )
			{
				const int itemType = optionalEntity->skill[21];
				messagePlayerColor(player, MESSAGE_COMBAT, color, msgGeneric, getMonsterLocalizedName((Monster)monsterType).c_str(), items[itemType].getIdentifiedName());
			}
			break;
		}
	}
	else
	{
		// use monster's "name" and pronoun grammar. "You hit Funny Bones!"
		switch (detailType)
		{
		case MSG_DESCRIPTION:
			if ( namedMonsterAsGeneric )
			{
				messagePlayerColor(player, MESSAGE_INSPECTION, color, msgGeneric, monsterStats.name);
			}
			else
			{
				messagePlayerColor(player, MESSAGE_INSPECTION, color, msgNamed, getMonsterLocalizedName((Monster)monsterType).c_str(), monsterStats.name);
			}
			break;
		case MSG_COMBAT:
			if ( namedMonsterAsGeneric )
			{
				messagePlayerColor(player, MESSAGE_COMBAT, color, msgGeneric, monsterStats.name);
			}
			else
			{
				messagePlayerColor(player, MESSAGE_COMBAT, color, msgNamed, monsterStats.name);
			}
			break;
		case MSG_COMBAT_BASIC:
			if ( namedMonsterAsGeneric )
			{
				messagePlayerColor(player, MESSAGE_COMBAT_BASIC, color, msgGeneric, monsterStats.name);
			}
			else
			{
				messagePlayerColor(player, MESSAGE_COMBAT_BASIC, color, msgNamed, monsterStats.name);
			}
			break;
		case MSG_OBITUARY:
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
						messagePlayerColor(c, MESSAGE_OBITUARY, color, msgNamed, monsterStats.name, monsterStats.obituary);
					}
					else if ( multiplayer != SINGLE )
					{
						messagePlayerColor(c, MESSAGE_OBITUARY, color, msgGeneric, stats[player]->name, monsterStats.name, monsterStats.obituary);
					}
				}
				else
				{
					if ( c == player || multiplayer != SINGLE )
					{
						messagePlayerColor(c, MESSAGE_OBITUARY, color, "%s %s", monsterStats.name, monsterStats.obituary);
					}
				}
			}
			break;
		default:
		case MSG_GENERIC:
			if ( namedMonsterAsGeneric || monsterType == HUMAN || (optionalEntity && optionalEntity->behavior == &actPlayer) )
			{
				messagePlayerColor(player, MESSAGE_HINT, color, msgGeneric, monsterStats.name);
			}
			else
			{
				messagePlayerColor(player, MESSAGE_HINT, color, msgGeneric, getMonsterLocalizedName((Monster)monsterType).c_str());
			}
			break;
		case MSG_ATTACKS:
			if ( namedMonsterAsGeneric )
			{
				messagePlayerColor(player, MESSAGE_COMBAT_BASIC, color, msgGeneric, monsterStats.name, getMonsterLocalizedInjury((Monster)monsterType).c_str());
			}
			else
			{
				messagePlayerColor(player, MESSAGE_COMBAT_BASIC, color, msgNamed, monsterStats.name, getMonsterLocalizedInjury((Monster)monsterType).c_str());
			}
			break;
		case MSG_STEAL_WEAPON:
			if ( monsterStats.weapon )
			{
				if ( namedMonsterAsGeneric )
				{
					messagePlayerColor(player, MESSAGE_COMBAT, color, msgGeneric, monsterStats.name, monsterStats.weapon->getName());
				}
				else
				{
					messagePlayerColor(player, MESSAGE_COMBAT, color, msgNamed, monsterStats.name, monsterStats.weapon->getName());
				}
			}
			break;
		case MSG_TOOL_BOMB:
			int itemType = WOODEN_SHIELD;
			if ( optionalEntity && optionalEntity->behavior == &actBomb )
			{
				itemType = optionalEntity->skill[21];
				if ( namedMonsterAsGeneric || monsterType == HUMAN )
				{
					messagePlayerColor(player, MESSAGE_COMBAT, color, msgGeneric, monsterStats.name, items[itemType].getIdentifiedName());
				}
				else
				{
					messagePlayerColor(player, MESSAGE_COMBAT, color, msgGeneric, getMonsterLocalizedName((Monster)monsterType).c_str(), items[itemType].getIdentifiedName());
				}
			}
			break;
		}
	}
}

/*-------------------------------------------------------------------------------

playerClassLangEntry
get text string for the different player chosen classes.

-------------------------------------------------------------------------------*/

char const * playerClassLangEntry(int classnum, int playernum)
{
	if ( classnum >= CLASS_BARBARIAN && classnum <= CLASS_JOKER )
	{
		return Language::get(1900 + classnum);
	}
	else if ( classnum >= CLASS_CONJURER )
	{
		return Language::get(3223 + classnum - CLASS_CONJURER);
	}
	else if ( classnum >= CLASS_SEXTON && classnum <= CLASS_MONK )
	{
		return Language::get(2550 + classnum - CLASS_SEXTON);
	}
	else
	{
		return "undefined classname";
	}
}

/*-------------------------------------------------------------------------------

setHelmetLimbOffset
Adjusts helmet offsets for all monsters, depending on the type of headwear.

-------------------------------------------------------------------------------*/

void Entity::setHelmetLimbOffset(Entity* helm)
{
	helm->scalex = 1.01;
	helm->scaley = 1.01;
	helm->scalez = 1.01;
	// for non-armor helmets, they are rotated so focaly acts as up/down postion.
	int monster = getMonsterTypeFromSprite();
	if ( EquipmentModelOffsets.modelOffsetExists(monster, helm->sprite) )
	{
		auto& entry = EquipmentModelOffsets.getModelOffset(monster, helm->sprite);
		helm->focalx = limbs[monster][entry.limbsIndex][0] + entry.focalx;
		helm->focaly = limbs[monster][entry.limbsIndex][1] + entry.focaly;
		helm->focalz = limbs[monster][entry.limbsIndex][2] + entry.focalz;
		helm->scalex += entry.scalex;
		helm->scaley += entry.scaley;
		helm->scalez += entry.scalez;
		helm->roll = entry.rotation;
		helm->pitch += entry.pitch;
	}
	else if ( helm->sprite == items[HAT_PHRYGIAN].index )
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
	else if ( (helm->sprite >= items[HAT_HOOD].index && helm->sprite < items[HAT_HOOD].index + items[HAT_HOOD].variations)
		|| helm->sprite == items[HAT_HOOD_RED].index || helm->sprite == items[HAT_HOOD_SILVER].index 
		|| helm->sprite == items[PUNISHER_HOOD].index
		|| (helm->sprite >= items[HAT_HOOD_APPRENTICE].index && helm->sprite < items[HAT_HOOD_APPRENTICE].index + items[HAT_HOOD_APPRENTICE].variations)
		|| (helm->sprite >= items[HAT_HOOD_ASSASSIN].index && helm->sprite < items[HAT_HOOD_ASSASSIN].index + items[HAT_HOOD_ASSASSIN].variations)
		|| (helm->sprite >= items[HAT_HOOD_WHISPERS].index && helm->sprite < items[HAT_HOOD_WHISPERS].index + items[HAT_HOOD_WHISPERS].variations) )
	{
		switch ( monster )
		{
			case AUTOMATON:
			case SKELETON:
				helm->focalx = limbs[monster][9][0] - .5;
				helm->focaly = limbs[monster][9][1] - 2.5;
				helm->focalz = limbs[monster][9][2] + 2.25;
				if ( helm->sprite == (items[HAT_HOOD].index + 2)
					|| helm->sprite == (items[HAT_HOOD_ASSASSIN].index) )
				{
					helm->focaly += 0.5; // black hood
				}
				else if ( helm->sprite == (items[HAT_HOOD].index + 3) 
					|| helm->sprite == (items[HAT_HOOD_APPRENTICE].index) )
				{
					helm->focaly -= 0.5; // purple hood
				}
				else if ( helm->sprite == items[PUNISHER_HOOD].index )
				{
					helm->focalx += 0.25;
					helm->focaly += 0.5;
				}
				break;
			case INCUBUS:
			case SUCCUBUS:
				helm->focalx = limbs[monster][9][0] - .5;
				helm->focaly = limbs[monster][9][1] - 2.5;
				helm->focalz = limbs[monster][9][2] + 2.5;
				if ( helm->sprite == (items[HAT_HOOD].index + 3)
					|| helm->sprite == (items[HAT_HOOD_APPRENTICE].index) )
				{
					helm->focaly -= 0.5; // purple hood
				}
				else if ( helm->sprite == items[PUNISHER_HOOD].index )
				{
					if ( monster == INCUBUS )
					{
						helm->focalx += 0.25;
						helm->focaly += 0.25;
					}
				}
				break;
			case VAMPIRE:
			case SHOPKEEPER:
			case HUMAN:
				helm->focalx = limbs[monster][9][0] - .5;
				helm->focaly = limbs[monster][9][1] - 2.5;
				helm->focalz = limbs[monster][9][2] + 2.25;
				if ( helm->sprite == items[PUNISHER_HOOD].index )
				{
					helm->focaly += 0.25;
				}
				break;
			case GOATMAN:
				helm->focalx = limbs[monster][9][0] - .5;
				helm->focaly = limbs[monster][9][1] - 2.75;
				helm->focalz = limbs[monster][9][2] + 2.75;
				if ( helm->sprite == (items[HAT_HOOD].index + 2)
					|| helm->sprite == (items[HAT_HOOD_ASSASSIN].index) )
				{
					helm->focaly -= 0.25; // black hood
				}
				else if ( helm->sprite == (items[HAT_HOOD].index + 3)
					|| helm->sprite == (items[HAT_HOOD_APPRENTICE].index) )
				{
					helm->focaly -= 0.5; // purple hood
				}
				break;
			case INSECTOID:
				helm->focalx = limbs[monster][9][0] - .5;
				helm->focaly = limbs[monster][9][1] - 2.15;
				helm->focalz = limbs[monster][9][2] + 2.25;
				if ( helm->sprite == (items[HAT_HOOD].index + 2)
					|| helm->sprite == (items[HAT_HOOD_ASSASSIN].index) )
				{
					helm->focaly += 0.25; // black hood
				}
				else if ( helm->sprite == (items[HAT_HOOD].index + 3)
					|| helm->sprite == (items[HAT_HOOD_APPRENTICE].index) )
				{
					helm->focaly -= 0.5; // purple hood
				}
				else if ( helm->sprite == items[PUNISHER_HOOD].index )
				{
					helm->focalx += 0.5;
					helm->focaly += 0.15;
				}
				break;
			case GOBLIN:
			case SHADOW:
				helm->focalx = limbs[monster][9][0] - .5;
				helm->focaly = limbs[monster][9][1] - 2.75;
				helm->focalz = limbs[monster][9][2] + 2.5;
				if ( monster == GOBLIN && (this->sprite == 752 || this->sprite == 1039) ) // special female offset.
				{
					if ( helm->sprite == (items[HAT_HOOD].index + 3)
						|| helm->sprite == (items[HAT_HOOD_APPRENTICE].index) )
					{
						helm->focaly -= 0.5; // purple hood
					}
				}
				if ( helm->sprite == items[PUNISHER_HOOD].index )
				{
					helm->focaly += 0.25;
				}
				break;
			default:
				break;
		}
		/*helm->focalx += limbs[HUMAN][12][0];
		helm->focaly += limbs[HUMAN][12][1];
		helm->focalz += limbs[HUMAN][12][2];*/
		helm->roll = PI / 2;
	}
	else if ( helm->sprite == items[HAT_WIZARD].index 
		|| helm->sprite == items[HAT_JESTER].index )
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
				helm->focaly = limbs[monster][9][1] - 5.1;
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
				if ( monster == GOBLIN && (this->sprite == 752 || this->sprite == 1039) ) // special female offset.
				{
					helm->focaly -= 0.25;
				}
				break;
			default:
				break;
		}
		helm->roll = PI / 2;
	}
	else if ( helm->sprite == items[MASK_SHAMAN].index )
	{
		switch ( monster )
		{
			case AUTOMATON:
				helm->focalx = limbs[monster][10][0] + 1.f;
				helm->focaly = limbs[monster][10][1] - 0.5;
				helm->focalz = limbs[monster][10][2] - 1.5;
				break;
			case SKELETON:
				helm->focalx = limbs[monster][10][0] + 0.5;
				helm->focaly = limbs[monster][10][1] - 0.5;
				helm->focalz = limbs[monster][10][2] - 1.7;
				break;
			case INCUBUS:
				helm->focalx = limbs[monster][10][0] + 0.5;
				helm->focaly = limbs[monster][10][1] - 0.25;
				helm->focalz = limbs[monster][10][2] - 2;
				break;
			case SUCCUBUS:
				helm->focalx = limbs[monster][10][0] + 0.5;
				helm->focaly = limbs[monster][10][1] - 0;
				helm->focalz = limbs[monster][10][2] - 2.25;
				break;
			case VAMPIRE:
			case SHOPKEEPER:
			case HUMAN:
				helm->focalx = limbs[monster][10][0] + 0.75;
				helm->focaly = limbs[monster][10][1] - 0;
				helm->focalz = limbs[monster][10][2] - 2;
				break;
			case GOATMAN:
				helm->focalx = limbs[monster][10][0] + 0.7;
				helm->focaly = limbs[monster][10][1] + 0.25;
				helm->focalz = limbs[monster][10][2] - 2.55;
				break;
			case INSECTOID:
				helm->focalx = limbs[monster][10][0] + 1.03;
				helm->focaly = limbs[monster][10][1] - 0.25;
				helm->focalz = limbs[monster][10][2] - 1.5;
				break;
			case GOBLIN:
				helm->focalx = limbs[monster][10][0] + 0.7;
				helm->focaly = limbs[monster][10][1] + 0;
				helm->focalz = limbs[monster][10][2] - 2.25;
				//if ( monster == GOBLIN && (this->sprite == 752 || this->sprite == 1039) ) // special female offset.
				//{
				//	helm->focaly -= 0.25;
				//}
				break;
			case SHADOW:
			default:
				break;
		}
	}
	else
	{
		if ( helm->sprite == items[IRON_HELM].index )
		{
			helm->focalz -= 0.25;
		}
		if ( monster == GOBLIN && (this->sprite == 752 || this->sprite == 1039) ) // special female offset.
		{
			helm->focalz = limbs[monster][9][2] - 0.25; // all non-hat helms
		}
	}
}

real_t Entity::yawDifferenceFromEntity(Entity* entity)
{
	if ( entity )
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
		return (PI - abs(abs(entity->yaw - targetYaw) - PI)) * 2;
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
	setSpriteAttributes(entity, nullptr, nullptr);
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

	if ( !entity )
	{
		return nullptr;
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

	createChestInventory(entity, entity->chestType);

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

void Entity::addToWorldUIList(list_t *list)
{
	//printlog("*ATTEMPTING* to add Dennis to creature list.");
	if ( list )
	{
		if ( myWorldUIListNode )
		{
			list_RemoveNode(myWorldUIListNode);
			myWorldUIListNode = nullptr;
		}
		myWorldUIListNode = list_AddNodeLast(list);
		myWorldUIListNode->element = this;
		myWorldUIListNode->deconstructor = &emptyDeconstructor;
		myWorldUIListNode->size = sizeof(Entity);
		//printlog("Added dennis to creature list.");
	}
}

int Entity::getMagicResistance(Stat* myStats)
{
	int resistance = 0;
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
	auto& rng = entity_rng ? *entity_rng : local_rng;

	if ( (svFlags & SV_FLAG_HARDCORE) && stats.MISC_FLAGS[STAT_FLAG_MONSTER_DISABLE_HC_SCALING] == 0 )
	{
		// spice up some stats...
		int statIncrease = ((abs(stats.HP) / 20 + 1) * 20); // each 20 HP add 20 random HP
		stats.HP += statIncrease - (rng.rand() % (std::max(statIncrease / 5, 1))); // 80%-100% of increased value
		stats.MAXHP = stats.HP;
		stats.OLDHP = stats.HP;

		statIncrease = (abs(stats.STR) / 5 + 1) * 5; // each 5 STR add 5 more STR.
		stats.STR += (statIncrease - (rng.rand() % (std::max(statIncrease / 4, 1)))); // 75%-100% of increased value.

		statIncrease = (abs(stats.PER) / 5 + 1) * 5; // each 5 PER add 5 more PER.
		stats.PER += (statIncrease - (rng.rand() % (std::max(statIncrease / 4, 1)))); // 75%-100% of increased value.

		statIncrease = std::min((abs(stats.DEX) / 4 + 1) * 1, 8); // each 4 DEX add 1 more DEX, capped at 8.
		stats.DEX += (statIncrease - (rng.rand() % (std::max(statIncrease / 2, 1)))); // 50%-100% of increased value.

		statIncrease = (abs(stats.CON) / 5 + 1) * 1; // each 5 CON add 1 more CON.
		stats.CON += (statIncrease - (rng.rand() % (std::max(statIncrease / 2, 1)))); // 50%-100% of increased value.

		statIncrease = (abs(stats.INT) / 5 + 1) * 5; // each 5 INT add 5 more INT.
		stats.INT += (statIncrease - (rng.rand() % (std::max(statIncrease / 2, 1)))); // 50%-100% of increased value.

		int lvlIncrease = rng.rand() % 4;
		lvlIncrease = std::max(0, lvlIncrease - 1);
		stats.LVL += std::max(0, lvlIncrease - 1); // increase by 1 or 2 50%, else stay same.
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
	if ( monsterStats.MISC_FLAGS[STAT_FLAG_MONSTER_NAME_GENERIC] == 1 )
	{
		return true;
	}
	if ( strstr(monsterStats.name, "lesser")
		|| strstr(monsterStats.name, "young")
		|| strstr(monsterStats.name, "enslaved")
		|| strstr(monsterStats.name, "damaged")
		|| strstr(monsterStats.name, "corrupted")
		|| strstr(monsterStats.name, "cultist")
		|| strstr(monsterStats.name, "knight")
		|| strstr(monsterStats.name, "sentinel")
		|| strstr(monsterStats.name, "mage")
		|| strstr(monsterStats.name, "inner")
		|| strstr(monsterStats.name, "training")
		|| strstr(monsterStats.name, "Training")
		|| strstr(monsterStats.name, "Mysterious")
		|| strstr(monsterStats.name, "shaman")
		|| !strcmp(monsterStats.name, Language::get(6302)) // gnome thief
		|| !strcmp(monsterStats.name, Language::get(6303)) // gnome thief leader
		|| strstr(monsterStats.name, getMonsterLocalizedName(SLIME).c_str()) )
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

	if ( static_cast<Sint32>(entity.getUID()) <= -3 )
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
	if ( limbType == LIMB_HUMANOID_TORSO )
	{
		limb->scalez = 1.f; // reset this scale incase something modifies this.
	}
	switch ( race )
	{
		case GNOME:
			if ( limbType == LIMB_HUMANOID_TORSO )
			{
				limb->x -= .25 * cos(this->yaw);
				limb->y -= .25 * sin(this->yaw);
				limb->z += 1.25;

				if ( limb->sprite == 1431 )
				{
					limb->focalx += 0.25;
				}
				else if ( limb->sprite == 296 )
				{
					limb->focalx += 0.5;
				}

				if ( limb->sprite != 1427 && limb->sprite != 1431 && limb->sprite != 296 )
				{
					limb->focalz -= 0.25;
				}
				if (limb->sprite == items[MACHINIST_APRON].indexShort)
				{
					limb->focalx -= 0.25;
					limb->focalz += 0.5;
				}

				limb->scalex = limbs[GNOME][11][0];
				limb->scaley = limbs[GNOME][11][1];
				limb->scalez = limbs[GNOME][11][2];
			}
			else if ( limbType == LIMB_HUMANOID_RIGHTLEG )
			{
				limb->x += 1.25 * cos(this->yaw + PI / 2);
				limb->y += 1.25 * sin(this->yaw + PI / 2);
				limb->z += 2.25;
				if ( limb->sprite == 1469 || limb->sprite == 1470 )
				{
					limb->focalx += 0.5;
				}
				if ( this->z >= 3.9 && this->z <= 4.1 )
				{
					limb->yaw += PI / 8;
					limb->pitch = -PI / 2;
				}
			}
			else if ( limbType == LIMB_HUMANOID_LEFTLEG )
			{
				limb->x -= 1.25 * cos(this->yaw + PI / 2);
				limb->y -= 1.25 * sin(this->yaw + PI / 2);
				limb->z += 2.25;
				if ( limb->sprite == 1469 || limb->sprite == 1470 )
				{
					limb->focalx += 0.5;
				}
				if ( this->z >= 3.9 && this->z <= 4.1 )
				{
					limb->yaw -= PI / 8;
					limb->pitch = -PI / 2;
				}
			}
			else if ( limbType == LIMB_HUMANOID_RIGHTARM )
			{
				limb->x += 2.5 * cos(this->yaw + PI / 2) - .75 * cos(this->yaw);
				limb->y += 2.5 * sin(this->yaw + PI / 2) - .75 * sin(this->yaw);
				limb->z -= .25;
				if ( this->z >= 3.9 && this->z <= 4.1 )
				{
					limb->pitch = 0;
				}

				if ( limb->sprite != 1434
					&& limb->sprite != 299 )
				{
					limb->x -= 0.25 * cos(this->yaw + PI / 2);
					limb->y -= 0.25 * sin(this->yaw + PI / 2);
				}
				if ( limb->sprite == 1434 )
				{
					limb->focalz -= 0.25;
				}
			}
			else if ( limbType == LIMB_HUMANOID_LEFTARM )
			{
				limb->x -= 2.5 * cos(this->yaw + PI / 2) + .75 * cos(this->yaw);
				limb->y -= 2.5 * sin(this->yaw + PI / 2) + .75 * sin(this->yaw);
				limb->z -= .25;
				if ( this->z >= 3.9 && this->z <= 4.1 )
				{
					limb->pitch = 0;
				}

				if ( limb->sprite != 1436
					&& limb->sprite != 301 )
				{
					limb->x += 0.25 * cos(this->yaw + PI / 2);
					limb->y += 0.25 * sin(this->yaw + PI / 2);
				}
				if ( limb->sprite == 1436 )
				{
					limb->focalz -= 0.25;
				}
			}
			break;
		case CREATURE_IMP:
			if ( limbType == LIMB_HUMANOID_TORSO )
			{
				limb->x -= 2 * cos(this->yaw);
				limb->y -= 2 * sin(this->yaw);
				limb->z += 2.75;
				limb->focalz -= 0.25;
			}
			else if ( limbType == LIMB_HUMANOID_RIGHTLEG )
			{
				limb->x += 1 * cos(this->yaw + PI / 2);
				limb->y += 1 * sin(this->yaw + PI / 2);
				limb->z += 6;
			}
			else if ( limbType == LIMB_HUMANOID_LEFTLEG )
			{
				limb->x -= 1 * cos(this->yaw + PI / 2);
				limb->y -= 1 * sin(this->yaw + PI / 2);
				limb->z += 6;
			}
			else if ( limbType == LIMB_HUMANOID_RIGHTARM )
			{
				limb->x += 3 * cos(this->yaw + PI / 2) - 1 * cos(this->yaw);
				limb->y += 3 * sin(this->yaw + PI / 2) - 1 * sin(this->yaw);
				limb->z += 1;
			}
			else if ( limbType == LIMB_HUMANOID_LEFTARM )
			{
				limb->x -= 3 * cos(this->yaw + PI / 2) + 1 * cos(this->yaw);
				limb->y -= 3 * sin(this->yaw + PI / 2) + 1 * sin(this->yaw);
				limb->z += 1;
			}
			break;
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
		case TROLL:
			if ( limbType == LIMB_HUMANOID_TORSO )
			{
				limb->x -= .5 * cos(this->yaw);
				limb->y -= .5 * sin(this->yaw);
				limb->z += 2.25;
			}
			else if ( limbType == LIMB_HUMANOID_RIGHTLEG )
			{
				limb->x += 2 * cos(this->yaw + PI / 2) - 0.75 * cos(this->yaw);
				limb->y += 2 * sin(this->yaw + PI / 2) - 0.75 * sin(this->yaw);
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
				limb->x -= 2 * cos(this->yaw + PI / 2) + 0.75 * cos(this->yaw);
				limb->y -= 2 * sin(this->yaw + PI / 2) + 0.75 * sin(this->yaw);
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
				limb->x += 3.5 * cos(this->yaw + PI / 2) - 1 * cos(this->yaw);
				limb->y += 3.5 * sin(this->yaw + PI / 2) - 1 * sin(this->yaw);
				limb->z += .1;
				//limb->yaw += MONSTER_WEAPONYAW;
				if ( this->z >= 1.4 && this->z <= 1.6 )
				{
					limb->pitch = 0;
				}
			}
			else if ( limbType == LIMB_HUMANOID_LEFTARM )
			{
				limb->x -= 3.5 * cos(this->yaw + PI / 2) + 1 * cos(this->yaw);
				limb->y -= 3.5 * sin(this->yaw + PI / 2) + 1 * sin(this->yaw);
				limb->z += .1;
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
					limb->x += .1 * cos(this->yaw);
					limb->y += .1 * sin(this->yaw);
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
				    && limb->sprite != 1046 && limb->sprite != 1048
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
				    && limb->sprite != 1045 && limb->sprite != 1047
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
					if ( limb->sprite != 727 && limb->sprite != 458 
						&& limb->sprite != 761
						&& limb->sprite != 1060 )
					{
						// wearing armor, offset by 1.
						limb->z -= 1;
					}
				}

				/*if ( limb->sprite == items[WIZARD_DOUBLET].index
					|| limb->sprite == items[HEALER_DOUBLET].index
					|| limb->sprite == items[TUNIC].index
					|| limb->sprite == items[TUNIC].index + 1 )
				{
					limb->z += 0.25;
				}*/
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

				if ( limb->sprite == items[WIZARD_DOUBLET].index
					|| limb->sprite == items[HEALER_DOUBLET].index
					|| limb->sprite == items[TUNIC].index
					|| limb->sprite == items[TUNIC].index + 1 )
				{
					limb->z += 0.5;
				}
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

	shieldLimb->scalex = 1.f;
	shieldLimb->scaley = 1.f;
	shieldLimb->scalez = 1.f;

	switch ( race )
	{
		case BUGBEAR:
			shieldLimb->x -= limbs[race][19][0] * cos(this->yaw + PI / 2) + limbs[race][19][1] * cos(this->yaw);
			shieldLimb->y -= limbs[race][19][0] * sin(this->yaw + PI / 2) + limbs[race][19][1] * sin(this->yaw);
			shieldLimb->z += limbs[race][19][2];
			shieldLimb->yaw = shieldArmLimb->yaw;
			shieldLimb->roll = 0;
			shieldLimb->pitch = shieldArmLimb->pitch;
		break;
		case CREATURE_IMP:
			shieldLimb->focalx = limbs[race][8][0];
			shieldLimb->focaly = limbs[race][8][1];
			shieldLimb->focalz = limbs[race][8][2];

			shieldLimb->x -= 2.5 * cos(this->yaw + PI / 2) + .20 * cos(this->yaw);
			shieldLimb->y -= 2.5 * sin(this->yaw + PI / 2) + .20 * sin(this->yaw);
			shieldLimb->z += 2.5;
			shieldLimb->yaw = shieldArmLimb->yaw;
			shieldLimb->roll = 0;
			shieldLimb->pitch = 0;

			if ( shieldLimb->sprite >= items[SPELLBOOK_LIGHT].index
				&& shieldLimb->sprite < (items[SPELLBOOK_LIGHT].index + items[SPELLBOOK_LIGHT].variations) )
			{
				shieldLimb->pitch = shieldArmLimb->pitch - .35 + 3 * PI / 2;
				shieldLimb->yaw += PI / 6;
				shieldLimb->focalx -= 4;
				shieldLimb->focalz += .5;
				shieldLimb->x += 0.5 * cos(this->yaw + PI / 2) - 1 * cos(this->yaw);
				shieldLimb->y += 0.5 * sin(this->yaw + PI / 2) - 1 * sin(this->yaw);
				shieldLimb->z -= 0.5;
				shieldLimb->scalex = 0.8;
				shieldLimb->scaley = 0.8;
				shieldLimb->scalez = 0.8;
			}
			break;
		case GNOME:
			shieldLimb->x -= 2.5 * cos(this->yaw + PI / 2) + .20 * cos(this->yaw);
			shieldLimb->y -= 2.5 * sin(this->yaw + PI / 2) + .20 * sin(this->yaw);
			shieldLimb->z += 1;
			shieldLimb->yaw = shieldArmLimb->yaw;
			shieldLimb->roll = 0;
			shieldLimb->pitch = 0;

			if ( shieldLimb->sprite == items[TOOL_LANTERN].index )
			{
				shieldLimb->z += 2;
			}
			if ( flickerLights || ticks % TICKS_PER_SECOND == 1 )
			{
				if ( shieldLimb->sprite == items[TOOL_TORCH].index )
				{
					if ( flameEntity = spawnFlame(shieldLimb, SPRITE_FLAME) )
					{
						flameEntity->x += 2 * cos(shieldArmLimb->yaw);
						flameEntity->y += 2 * sin(shieldArmLimb->yaw);
						flameEntity->z -= 2;
					}
				}
				else if ( shieldLimb->sprite == items[TOOL_CRYSTALSHARD].index )
				{
					/*flameEntity = spawnFlame(shieldLimb, SPRITE_CRYSTALFLAME);
					flameEntity->x += 2 * cos(shieldArmLimb->yaw);
					flameEntity->y += 2 * sin(shieldArmLimb->yaw);
					flameEntity->z -= 2;*/
				}
				else if ( shieldLimb->sprite == items[TOOL_LANTERN].index )
				{
					if ( flameEntity = spawnFlame(shieldLimb, SPRITE_FLAME) )
					{
						flameEntity->x += 2 * cos(shieldArmLimb->yaw);
						flameEntity->y += 2 * sin(shieldArmLimb->yaw);
						flameEntity->z += 1;
					}
				}
			}
			if ( itemSpriteIsQuiverThirdPersonModel(shieldLimb->sprite) )
			{
				shieldLimb->focalz += 3;
				shieldLimb->scalex = 1.05;
				shieldLimb->x -= -0.25 * cos(this->yaw + PI / 2) + 1.25 * cos(this->yaw);
				shieldLimb->y -= -0.25 * sin(this->yaw + PI / 2) + 1.25 * sin(this->yaw);
				shieldLimb->z += -1.28;
			}
			else if ( shieldLimb->sprite >= items[SPELLBOOK_LIGHT].index
				&& shieldLimb->sprite < (items[SPELLBOOK_LIGHT].index + items[SPELLBOOK_LIGHT].variations) )
			{
				shieldLimb->pitch = shieldArmLimb->pitch - .25 + 3 * PI / 2;
				shieldLimb->yaw += PI / 6;
				shieldLimb->focalx -= 4;
				shieldLimb->focalz += .5;
				shieldLimb->x += 0.5 * cos(this->yaw + PI / 2) + .5 * cos(this->yaw);
				shieldLimb->y += 0.5 * sin(this->yaw + PI / 2) + .5 * sin(this->yaw);
				shieldLimb->z -= 1;
				shieldLimb->scalex = 0.8;
				shieldLimb->scaley = 0.8;
				shieldLimb->scalez = 0.8;
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
		case HUMAN:
		case VAMPIRE:
			shieldLimb->x -= 2.5 * cos(this->yaw + PI / 2) + .20 * cos(this->yaw);
			shieldLimb->y -= 2.5 * sin(this->yaw + PI / 2) + .20 * sin(this->yaw);
			shieldLimb->z += 2.5;
			shieldLimb->yaw = shieldArmLimb->yaw;
			shieldLimb->roll = 0;
			shieldLimb->pitch = 0;

			if ( shieldLimb->sprite == items[TOOL_LANTERN].index )
			{
			    shieldLimb->z += 2;
			}
	        if ( flickerLights || ticks % TICKS_PER_SECOND == 1 )
	        {
			    if ( shieldLimb->sprite == items[TOOL_TORCH].index )
			    {
					if ( flameEntity = spawnFlame(shieldLimb, SPRITE_FLAME) )
					{
						flameEntity->x += 2 * cos(shieldArmLimb->yaw);
						flameEntity->y += 2 * sin(shieldArmLimb->yaw);
						flameEntity->z -= 2;
					}
			    }
			    else if ( shieldLimb->sprite == items[TOOL_CRYSTALSHARD].index )
			    {
				    /*flameEntity = spawnFlame(shieldLimb, SPRITE_CRYSTALFLAME);
				    flameEntity->x += 2 * cos(shieldArmLimb->yaw);
				    flameEntity->y += 2 * sin(shieldArmLimb->yaw);
				    flameEntity->z -= 2;*/
			    }
			    else if ( shieldLimb->sprite == items[TOOL_LANTERN].index )
			    {
					if ( flameEntity = spawnFlame(shieldLimb, SPRITE_FLAME) )
					{
						flameEntity->x += 2 * cos(shieldArmLimb->yaw);
						flameEntity->y += 2 * sin(shieldArmLimb->yaw);
						flameEntity->z += 1;
					}
			    }
			}
			if ( itemSpriteIsQuiverThirdPersonModel(shieldLimb->sprite) )
			{
				shieldLimb->focalz += 3;
				shieldLimb->scalex = 1.05;
				shieldLimb->x -= -0.25 * cos(this->yaw + PI / 2) + 1.25 * cos(this->yaw);
				shieldLimb->y -= -0.25 * sin(this->yaw + PI / 2) + 1.25 * sin(this->yaw);
				shieldLimb->z += -1.28;
			}
			else if ( shieldLimb->sprite >= items[SPELLBOOK_LIGHT].index
				&& shieldLimb->sprite < (items[SPELLBOOK_LIGHT].index + items[SPELLBOOK_LIGHT].variations) )
			{
				shieldLimb->pitch = shieldArmLimb->pitch - .25 + 3 * PI / 2;
				shieldLimb->yaw += PI / 6;
				shieldLimb->focalx -= 4;
				shieldLimb->focalz += .5;
				shieldLimb->x += 0.5 * cos(this->yaw + PI / 2) + .5 * cos(this->yaw);
				shieldLimb->y += 0.5 * sin(this->yaw + PI / 2) + .5 * sin(this->yaw);
				shieldLimb->z -= 1;
				shieldLimb->scalex = 0.8;
				shieldLimb->scaley = 0.8;
				shieldLimb->scalez = 0.8;
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
			if ( itemSpriteIsQuiverThirdPersonModel(shieldLimb->sprite) )
			{
				shieldLimb->focalz += 3;
				shieldLimb->z += -1.78;
				shieldLimb->scalex = 1.05;
				if ( race == SKELETON )
				{
					shieldLimb->x -= -0.2 * cos(this->yaw + PI / 2) + 1.6 * cos(this->yaw);
					shieldLimb->y -= -0.2 * sin(this->yaw + PI / 2) + 1.6 * sin(this->yaw);
					if ( behavior == &actMonster )
					{
						// additional offsets for skellie monsters since the player offsets slightly different
						shieldLimb->x += -0.25 * cos(this->yaw);
						shieldLimb->y += -0.25 * sin(this->yaw);
						shieldLimb->z += -0.25;
					}
				}
				else if ( race == AUTOMATON )
				{
					shieldLimb->x -= 0.1 * cos(this->yaw + PI / 2) + 2.1 * cos(this->yaw);
					shieldLimb->y -= 0.1 * sin(this->yaw + PI / 2) + 2.1 * sin(this->yaw);
					shieldLimb->z += 0.1;
					
					// originally shieldLimb->x/y was -1.1 * cos or sin but it fell outside the player bounding box and was visible in hud
					// so shieldLimb->x/y is -0.1 and -1 is added to focaly.
					shieldLimb->focaly -= 1; 
				}
			}

            if ( shieldLimb->sprite == items[TOOL_LANTERN].index )
			{
				shieldLimb->z += 2;
			}
	        if ( flickerLights || ticks % TICKS_PER_SECOND == 1 )
	        {
			    if ( shieldLimb->sprite == items[TOOL_TORCH].index )
			    {
					if ( flameEntity = spawnFlame(shieldLimb, SPRITE_FLAME) )
					{
						flameEntity->x += 2.5 * cos(shieldLimb->yaw + PI / 16);
						flameEntity->y += 2.5 * sin(shieldLimb->yaw + PI / 16);
						flameEntity->z -= 2;
					}
			    }
			    else if ( shieldLimb->sprite == items[TOOL_CRYSTALSHARD].index )
			    {
				    /*flameEntity = spawnFlame(shieldLimb, SPRITE_CRYSTALFLAME);
				    flameEntity->x += 2.5 * cos(shieldLimb->yaw + PI / 16);
				    flameEntity->y += 2.5 * sin(shieldLimb->yaw + PI / 16);
				    flameEntity->z -= 2;*/
			    }
			    else if ( shieldLimb->sprite == items[TOOL_LANTERN].index )
			    {
					if ( flameEntity = spawnFlame(shieldLimb, SPRITE_FLAME) )
					{
						flameEntity->x += 2.5 * cos(shieldLimb->yaw);
						flameEntity->y += 2.5 * sin(shieldLimb->yaw);
						flameEntity->z += 1;
					}
			    }
			}
			if ( shieldLimb->sprite >= items[SPELLBOOK_LIGHT].index
				&& shieldLimb->sprite < (items[SPELLBOOK_LIGHT].index + items[SPELLBOOK_LIGHT].variations) )
			{
				shieldLimb->pitch = shieldArmLimb->pitch - .25 + 3 * PI / 2;
				shieldLimb->yaw += PI / 16;
				shieldLimb->focalx -= 4;
				if ( race == SKELETON )
				{
					shieldLimb->focaly += .5;
				}
				else if ( race == AUTOMATON )
				{
					shieldLimb->focaly += .5;
					shieldLimb->focalz -= .5;
				}
				shieldLimb->focalz += .5;
				shieldLimb->x += 0.5 * cos(this->yaw + PI / 2) + .5 * cos(this->yaw);
				shieldLimb->y += 0.5 * sin(this->yaw + PI / 2) + .5 * sin(this->yaw);
				shieldLimb->z -= 1;
				shieldLimb->scalex = 0.8;
				shieldLimb->scaley = 0.8;
				shieldLimb->scalez = 0.8;
			}
			else
			{
				// automaton arm clips through shield. extend shield out more.
				if ( race == AUTOMATON )
				{
					if ( shieldLimb->sprite != items[TOOL_TINKERING_KIT].index
						&& shieldLimb->sprite != items[TOOL_TORCH].index
						&& shieldLimb->sprite != items[TOOL_CRYSTALSHARD].index
						&& shieldLimb->sprite != items[TOOL_LANTERN].index )
					{
						shieldLimb->focalx += 0.75;
						shieldLimb->focaly += 1.25;
					}
				}
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
					if ( !itemSpriteIsQuiverThirdPersonModel(shieldLimb->sprite) )
					{
						shieldArmLimb->yaw -= PI / 8;
					}
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

            if ( shieldLimb->sprite == items[TOOL_LANTERN].index )
			{
			    shieldLimb->z += 2;
			}
	        if ( flickerLights || ticks % TICKS_PER_SECOND == 1 )
	        {
		        if ( shieldLimb->sprite == items[TOOL_TORCH].index )
			    {
					if ( flameEntity = spawnFlame(shieldLimb, SPRITE_FLAME) )
					{
						flameEntity->x += 2 * cos(shieldLimb->yaw);
						flameEntity->y += 2 * sin(shieldLimb->yaw);
						flameEntity->z -= 2;
					}
			    }
			    else if ( shieldLimb->sprite == items[TOOL_CRYSTALSHARD].index )
			    {
				    /*flameEntity = spawnFlame(shieldLimb, SPRITE_CRYSTALFLAME);
				    flameEntity->x += 2 * cos(shieldLimb->yaw);
				    flameEntity->y += 2 * sin(shieldLimb->yaw);
				    flameEntity->z -= 2;*/
			    }
			    else if ( shieldLimb->sprite == items[TOOL_LANTERN].index )
			    {
					if ( flameEntity = spawnFlame(shieldLimb, SPRITE_FLAME) )
					{
						flameEntity->x += 2 * cos(shieldLimb->yaw);
						flameEntity->y += 2 * sin(shieldLimb->yaw);
						flameEntity->z += 1;
					}
			    }
			}
			if ( shieldLimb->sprite >= items[SPELLBOOK_LIGHT].index
				&& shieldLimb->sprite < (items[SPELLBOOK_LIGHT].index + items[SPELLBOOK_LIGHT].variations) )
			{
				shieldLimb->pitch = shieldArmLimb->pitch - .25 + 3 * PI / 2;
				shieldLimb->yaw += PI / 6;
				shieldLimb->focalx -= 4;
				shieldLimb->focalz += .5;
				if ( race == INCUBUS || race == SUCCUBUS )
				{
					shieldLimb->focalz -= 1.5;
				}
				shieldLimb->x += 0.5 * cos(this->yaw + PI / 2) + .5 * cos(this->yaw);
				shieldLimb->y += 0.5 * sin(this->yaw + PI / 2) + .5 * sin(this->yaw);
				shieldLimb->z -= 1;
				shieldLimb->scalex = 0.8;
				shieldLimb->scaley = 0.8;
				shieldLimb->scalez = 0.8;
			}
			else if ( itemSpriteIsQuiverThirdPersonModel(shieldLimb->sprite) )
			{
				shieldLimb->scalex = 1.05;
				//shieldLimb->scalez = 1.05;
				shieldLimb->focalz += 3;
				if ( race == INCUBUS || race == SUCCUBUS )
				{
					if ( race == SUCCUBUS )
					{
						shieldLimb->x -= 0.05 * cos(this->yaw + PI / 2) + (1.75) * cos(this->yaw);
						shieldLimb->y -= 0.05 * sin(this->yaw + PI / 2) + (1.75) * sin(this->yaw);
					}
					else
					{
						shieldLimb->x -= 0.05 * cos(this->yaw + PI / 2) + (1.5) * cos(this->yaw);
						shieldLimb->y -= 0.05 * sin(this->yaw + PI / 2) + (1.5) * sin(this->yaw);
					}
					shieldLimb->z += -2.28;
				}
				else
				{
					if ( race == GOATMAN )
					{
						shieldLimb->x -= -0.25 * cos(this->yaw + PI / 2) + 1.5 * cos(this->yaw);
						shieldLimb->y -= -0.25 * sin(this->yaw + PI / 2) + 1.5 * sin(this->yaw);
					}
					else if ( race == GOBLIN )
					{
						shieldLimb->x -= -0.25 * cos(this->yaw + PI / 2) + 1.25 * cos(this->yaw);
						shieldLimb->y -= -0.25 * sin(this->yaw + PI / 2) + 1.25 * sin(this->yaw);
					}
					else
					{
						shieldLimb->x -= -0.25 * cos(this->yaw + PI / 2) + 1.0 * cos(this->yaw);
						shieldLimb->y -= -0.25 * sin(this->yaw + PI / 2) + 1.0 * sin(this->yaw);
					}
					shieldLimb->z += -1.78;
				}
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

	if ( shieldLimb->sprite == items[TOOL_TINKERING_KIT].index )
	{
		//shieldLimb->pitch = 0;
		shieldLimb->yaw += PI / 6;
		shieldLimb->focalx -= .5;
		shieldLimb->focaly += .25;
		shieldLimb->focalz += 2.25;
		if ( race == INCUBUS || race == SUCCUBUS )
		{
			shieldLimb->focalx -= .5;
			shieldLimb->focaly -= 0.7;
		}
		else if ( race == GOATMAN || race == GOBLIN || race == INSECTOID )
		{
			shieldLimb->focalz -= 0.5;
		}
		else if ( race == AUTOMATON || race == SKELETON )
		{
			shieldLimb->focalz -= 1;
			if ( race == SKELETON )
			{
				shieldLimb->focalx -= 0.5;
			}
			if ( this->fskill[8] > PI / 32 ) //MONSTER_SHIELDYAW and PLAYER_SHIELDYAW defending animation
			{
				shieldLimb->focalx += 2;
			}
			else
			{
				shieldLimb->focalx += 1;
				shieldLimb->focaly -= 0.8;
			}
		}
	}
	else if ( itemSpriteIsQuiverThirdPersonModel(shieldLimb->sprite) )
	{
		shieldLimb->yaw += PI / 6;
	}

	if ( flameEntity && player >= 0 )
	{
		if ( players[player]->isLocalPlayer() )
		{
			flameEntity->flags[GENIUS] = true;
			flameEntity->setUID(-4);
			flameEntity->skill[1] = player + 1;
		}
		else
		{
			flameEntity->setUID(-3);
		}
	}
}

bool Entity::isBossMonster()
{
	Stat* myStats = getStats();
	if ( myStats )
	{
		if ( myStats->type == MINOTAUR
			|| myStats->type == SHOPKEEPER
			|| myStats->type == SHADOW
			|| myStats->type == LICH
			|| myStats->type == LICH_FIRE
			|| myStats->type == LICH_ICE
			|| myStats->type == DEVIL
			|| (myStats->type == VAMPIRE && MonsterData_t::nameMatchesSpecialNPCName(*myStats, "bram kindly"))
			|| (myStats->type == COCKATRICE && !strncmp(map.name, "Cockatrice Lair", 15))
			)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	return false;
}

void Entity::handleKnockbackDamage(Stat& myStats, Entity* knockedInto)
{
	if ( knockedInto != NULL && myStats.EFFECTS[EFF_KNOCKBACK] && myStats.HP > 0 )
	{
		int damageOnHit = 5 + local_rng.rand() % 6;
		if ( knockedInto->behavior == &actDoor )
		{
			playSoundEntity(this, 28, 64);
			this->modHP(-damageOnHit);
			Entity* whoKnockedMe = uidToEntity(this->monsterKnockbackUID);
			if ( myStats.HP <= 0 )
			{
				if ( whoKnockedMe )
				{
					whoKnockedMe->awardXP(this, true, true);
				}
			}
			if ( whoKnockedMe && whoKnockedMe->behavior == &actPlayer )
			{
				steamStatisticUpdateClient(whoKnockedMe->skill[2], STEAM_STAT_TAKE_THIS_OUTSIDE, STEAM_STAT_INT, 1);
				Compendium_t::Events_t::eventUpdateWorld(whoKnockedMe->skill[2], Compendium_t::CPDM_DOOR_BROKEN, "door", 1);
			}
			knockedInto->doorHealth = 0;    // smash doors instantly
			playSoundEntity(knockedInto, 28, 64);
			if ( knockedInto->doorHealth <= 0 )
			{
				// set direction of splinters
				if ( !knockedInto->doorDir )
				{
					knockedInto->doorSmacked = (this->x > knockedInto->x);
				}
				else
				{
					knockedInto->doorSmacked = (this->y < knockedInto->y);
				}
			}
		}
		else if ( knockedInto->behavior == &::actFurniture )
		{
			// break it down!
			playSoundEntity(this, 28, 64);
			this->modHP(-damageOnHit);
			if ( myStats.HP <= 0 )
			{
				Entity* whoKnockedMe = uidToEntity(this->monsterKnockbackUID);
				if ( whoKnockedMe )
				{
					whoKnockedMe->awardXP(this, true, true);
				}
			}
			knockedInto->furnitureHealth = 0;    // smash furniture instantly
			playSoundEntity(knockedInto, 28, 64);
		}
	}
}

void Entity::setHelmetLimbOffsetWithMask(Entity* helm, Entity* mask)
{
	if ( !helm || !mask )
	{
		return;
	}

	bool maskVisible = (!mask->flags[INVISIBLE]) || (mask->flags[INVISIBLE] && mask->flags[INVISIBLE_DITHER]);
	bool helmVisible = (!helm->flags[INVISIBLE]) || (helm->flags[INVISIBLE] && helm->flags[INVISIBLE_DITHER]);

	if ( maskVisible && helmVisible )
	{
		helm->scalex = 1.01;
		helm->scaley = 1.01;
		helm->scalez = 1.01;

		int monster = getMonsterTypeFromSprite();
		if ( EquipmentModelOffsets.modelOffsetExists(monster, helm->sprite) )
		{
			auto& entry = EquipmentModelOffsets.getModelOffset(monster, helm->sprite);
			helm->scalex += entry.scalex;
			helm->scaley += entry.scaley;
			helm->scalez += entry.scalez;
		}
	}
	else
	{
		mask->scalex = 1.01;
		mask->scaley = 1.01;
		mask->scalez = 1.01;
		return;
	}

	mask->scalex = 1.01;
	mask->scaley = 1.01;
	mask->scalez = 1.01;

	int monster = getMonsterTypeFromSprite();

	if ( helm->sprite == items[LEATHER_HELM].index
		|| helm->sprite == items[IRON_HELM].index
		|| (helm->sprite >= items[HAT_HOOD].index && helm->sprite < items[HAT_HOOD].index + items[HAT_HOOD].variations)
		|| helm->sprite == items[HAT_HOOD_RED].index
		|| helm->sprite == items[HAT_HOOD_SILVER].index
		|| (helm->sprite >= items[HAT_HOOD_APPRENTICE].index && helm->sprite < items[HAT_HOOD_APPRENTICE].index + items[HAT_HOOD_APPRENTICE].variations)
		|| (helm->sprite >= items[HAT_HOOD_ASSASSIN].index && helm->sprite < items[HAT_HOOD_ASSASSIN].index + items[HAT_HOOD_ASSASSIN].variations)
		|| (helm->sprite >= items[HAT_HOOD_WHISPERS].index && helm->sprite < items[HAT_HOOD_WHISPERS].index + items[HAT_HOOD_WHISPERS].variations)
		|| helm->sprite == items[PUNISHER_HOOD].index )
	{
		helm->scalex = 1.05;
		helm->scaley = 1.05;
		helm->scalez = 1.05;

		if ( EquipmentModelOffsets.maskHasAdjustmentForExpandedHelm(monster, helm->sprite, mask->sprite) )
		{
			auto offsetMask = EquipmentModelOffsets.getMaskOffsetForExpandHelm(monster, helm->sprite, mask->sprite);
			mask->focalx += offsetMask.focalx;
			mask->focaly += offsetMask.focaly;
			mask->focalz += offsetMask.focalz;
			mask->scalex += offsetMask.scalex;
			mask->scaley += offsetMask.scaley;
			mask->scalez += offsetMask.scalez;

			if ( mask->sprite == items[MASK_SPOOKY].index && (helm->sprite == items[LEATHER_HELM].index
				|| helm->sprite == items[IRON_HELM].index) )
			{
				helm->scalex = 1.01;
				helm->scaley = 1.01;
				helm->scalez = 1.01;
			}
		}
	}
	else if ( EquipmentModelOffsets.expandHelmToFitMask(monster, helm->sprite, mask->sprite) )
	{
		helm->scalex = 1.05;
		helm->scaley = 1.05;
		helm->scalez = 1.05;
		
		auto offsetHelm = EquipmentModelOffsets.getExpandHelmOffset(monster, helm->sprite, mask->sprite);
		helm->focalx += offsetHelm.focalx;
		helm->focaly += offsetHelm.focaly;
		helm->focalz += offsetHelm.focalz;
		helm->scalex += offsetHelm.scalex;
		helm->scaley += offsetHelm.scaley;
		helm->scalez += offsetHelm.scalez;

		auto offsetMask = EquipmentModelOffsets.getMaskOffsetForExpandHelm(monster, helm->sprite, mask->sprite);
		mask->focalx += offsetMask.focalx;
		mask->focaly += offsetMask.focaly;
		mask->focalz += offsetMask.focalz;
		mask->scalex += offsetMask.scalex;
		mask->scaley += offsetMask.scaley;
		mask->scalez += offsetMask.scalez;

		return;
	}
	else if ( EquipmentModelOffsets.maskHasAdjustmentForExpandedHelm(monster, helm->sprite, mask->sprite) )
	{
		auto offsetMask = EquipmentModelOffsets.getMaskOffsetForExpandHelm(monster, helm->sprite, mask->sprite);
		mask->focalx += offsetMask.focalx;
		mask->focaly += offsetMask.focaly;
		mask->focalz += offsetMask.focalz;
		mask->scalex += offsetMask.scalex;
		mask->scaley += offsetMask.scaley;
		mask->scalez += offsetMask.scalez;

		return;
	}

	switch ( monster )
	{
		case HUMAN:
		case VAMPIRE:
		case SHOPKEEPER:
			if ( helm->sprite == items[LEATHER_HELM].index )
			{
				helm->focalz -= 0.2;
			}
			break;
		case GOBLIN:
			if ( helm->sprite == items[LEATHER_HELM].index
				|| helm->sprite == items[IRON_HELM].index )
			{
				helm->focalz -= 0.2;
			}
			else if ( helm->sprite == (items[HAT_HOOD].index + 2)
				|| helm->sprite == (items[HAT_HOOD_ASSASSIN].index) )
			{
				// black hood
				helm->focalx += 0.25;
			}
			break;
		case GOATMAN:
			if ( helm->sprite == items[LEATHER_HELM].index
				|| helm->sprite == items[IRON_HELM].index )
			{
				helm->focalz -= 0.25;
			}
			else if ( helm->sprite == items[PUNISHER_HOOD].index )
			{
				helm->scaley += 0.05;
			}
			break;
		case INCUBUS:
			if ( helm->sprite == items[LEATHER_HELM].index
				|| helm->sprite == items[IRON_HELM].index )
			{
				helm->focalx += 0.4;
				helm->focalz -= 0.2;
			}
			else if ( helm->sprite == items[PUNISHER_HOOD].index )
			{
				mask->focalx -= 0.1;
			}
			break;
		case SUCCUBUS:
			if ( helm->sprite == items[LEATHER_HELM].index
				|| helm->sprite == items[IRON_HELM].index )
			{
				helm->focalz -= 0.2;
			}
			break;
		case AUTOMATON:
			if ( helm->sprite == items[LEATHER_HELM].index
				|| helm->sprite == items[IRON_HELM].index )
			{
				mask->focalx -= 0.2;
			}
			else if ( helm->sprite == items[PUNISHER_HOOD].index )
			{
				mask->focalx -= 0.2;
			}
			break;
		case INSECTOID:
			if ( helm->sprite == items[LEATHER_HELM].index )
			{
				helm->focalz -= 0.2;
			}
			else if ( helm->sprite == (items[HAT_HOOD].index + 2)
				|| helm->sprite == (items[HAT_HOOD_ASSASSIN].index) )
			{
				// black hood
				helm->focalx += 0.25;
				helm->focaly += 0.06;
			}
			else if ( helm->sprite == items[PUNISHER_HOOD].index )
			{
				mask->focalx -= 0.2;
			}
			break;
		case SKELETON:
			break;
		default:
			break;
	}
}

bool monsterIsImmobileTurret(Entity* my, Stat* myStats)
{
	if ( myStats )
	{
		if ( myStats->type == SENTRYBOT || myStats->type == SPELLBOT || myStats->type == DUMMYBOT )
		{
			return true;
		}
	}
	else if ( my )
	{
		int race = my->getMonsterTypeFromSprite();
		if ( race == SENTRYBOT || race == SPELLBOT || race == DUMMYBOT )
		{
			return true;
		}
	}

	return false;
}

bool monsterChangesColorWhenAlly(Stat* myStats, Entity* entity)
{
	int race = NOTHING;
	if ( !myStats )
	{
		if ( !entity )
		{
			return true;
		}
		race = entity->getMonsterTypeFromSprite();
	}
	else
	{
		race = myStats->type;
	}
	
	if ( race == HUMAN || race == SENTRYBOT || race == NOTHING || race == SLIME
		|| race == SPELLBOT || race == AUTOMATON || race == GYROBOT || race == DUMMYBOT )
	{
		return false;
	}
	return true;
}

int monsterTinkeringConvertHPToAppearance(Stat* myStats)
{
	if ( myStats )
	{
		if ( myStats->MAXHP == 0 || myStats->HP == 0 )
		{
			return 0;
		}
		if ( myStats->MAXHP == myStats->HP )
		{
			return ITEM_TINKERING_APPEARANCE;
		}
		real_t ratio = (1.0 * myStats->HP) / (myStats->MAXHP);
		if ( ratio >= 0.74 )
		{
			return 3;
		}
		else if ( ratio >= 0.49 )
		{
			return 2;
		}
		else if ( ratio >= 0.24 )
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}
	return 0;
}

int monsterTinkeringConvertAppearanceToHP(Stat* myStats, int appearance)
{
	if ( myStats )
	{
		if ( appearance == ITEM_TINKERING_APPEARANCE || (appearance > 0 && appearance % 10 == 0) )
		{
			return myStats->MAXHP;
		}
		int randomHP = std::max(1, myStats->MAXHP / 8);
		randomHP = randomHP + local_rng.rand() % randomHP;
		int convertedAppearance = appearance % 10;
		return std::min(myStats->MAXHP, ((convertedAppearance * myStats->HP) / 4) + randomHP);
	}
	return 0;
}

void Entity::handleQuiverThirdPersonModel(Stat& myStats)
{
	if ( multiplayer == CLIENT )
	{
		return;
	}
	if ( !myStats.breastplate )
	{
		switch ( myStats.type )
		{
			case SKELETON:
			case AUTOMATON:
				sprite += 2; // short strap
				break;
			case KOBOLD:
			case GNOME:
				// no strap.
				break;
			default:
				sprite += 1; // normal strap
				break;
		}
	}
	else
	{
		switch ( myStats.type )
		{
			case SKELETON:
			case AUTOMATON:
				sprite += 2; // short strap
				break;
			case KOBOLD:
			case GNOME:
				// no strap.
				break;
			default:
				sprite += 3; // shoulderpad-less.
				break;
		}
	}
}

Sint32 Entity::playerInsectoidExpectedManaFromHunger(Stat& myStats)
{
	real_t manaPercentFromHunger = myStats.HUNGER / 1000.f;
	real_t expectedManaValue = std::floor(myStats.MAXMP * manaPercentFromHunger);
	if ( myStats.HUNGER > 0 )
	{
		// add extra expected mana point here.
		// i.e 950 hunger is still full mana to avoid always having 1 short.
		// skip 0 hunger as it will be 0 expected.
		return (static_cast<Sint32>(expectedManaValue) + 1);
	}
	return static_cast<Sint32>(expectedManaValue);
}

Sint32 Entity::playerInsectoidHungerValueOfManaPoint(Stat& myStats)
{
	float manaPointPercentage = 1 / static_cast<float>(myStats.MAXMP);
	return static_cast<Sint32>(1000 * manaPointPercentage);
}

real_t Entity::getDamageTableMultiplier(Entity* my, Stat& myStats, DamageTableType damageType)
{
	real_t damageMultiplier = damagetables[myStats.type][damageType];
	real_t bonus = 0.0;
	if ( myStats.EFFECTS[EFF_SHADOW_TAGGED] )
	{
		if ( myStats.type == LICH || myStats.type == LICH_FIRE || myStats.type == LICH_ICE
			|| myStats.type == DEVIL )
		{
			return 1.5; // rough average.
		}
		else
		{
			return 1.0;
		}
	}
	//messagePlayer(0, "%f", damageMultiplier);
	if ( myStats.type == GOATMAN && myStats.EFFECTS[EFF_DRUNK] )
	{
		bonus = -.2;
	}
	if ( damageType == DamageTableType::DAMAGE_TABLE_MAGIC
		&& myStats.type == BUGBEAR && myStats.defending && myStats.shield )
	{
		damageMultiplier = 0.1;
	}
	int followerResist = my ? my->getFollowerBonusDamageResist() : 0;
	if ( followerResist != 0 )
	{
		bonus += -followerResist / 100.0;
	}
	return std::max(0.0, damageMultiplier + bonus);
}

void Entity::createWorldUITooltip()
{
	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		if ( !players[i]->isLocalPlayer() )
		{
			continue;
		}
		Entity* worldTooltip = nullptr;
#ifndef NINTENDO
		bool failedToAllocate = false;
		try
		{
			worldTooltip = new Entity(-1, 1, map.worldUI, nullptr);
		}
		catch ( std::bad_alloc& ba )
		{
			failedToAllocate = true;
		}

		if ( failedToAllocate || !worldTooltip )
		{
			printlog("failed to allocate memory for new entity!\n");
			exit(1);
		}
#else
		worldTooltip = new Entity(-1, 1, map.worldUI, nullptr);
#endif

		worldTooltip->x = this->x;
		worldTooltip->y = this->y;
		worldTooltip->z = this->z;
		worldTooltip->sizex = 1;
		worldTooltip->sizey = 1;
        worldTooltip->ditheringDisabled = true;
		worldTooltip->flags[NOUPDATE] = true;
		worldTooltip->flags[PASSABLE] = true;
		worldTooltip->flags[SPRITE] = true;
		worldTooltip->flags[UNCLICKABLE] = true;
		worldTooltip->behavior = &actSpriteWorldTooltip;
		worldTooltip->parent = this->getUID();
		worldTooltip->scalex = 0.05;
		worldTooltip->scaley = 0.05;
		worldTooltip->scalez = 0.05;
		worldTooltip->worldTooltipPlayer = i;
		worldTooltip->worldTooltipZ = 1.5;
		players[i]->worldUI.setTooltipDisabled(*worldTooltip);
		//worldTooltip->addToWorldUIList(map.worldUI);

		if ( behavior != &actItem )
		{
			worldTooltip->worldTooltipIgnoreDrawing = 1;
		}

		if ( bEntityTooltipRequiresButtonHeld() )
		{
			worldTooltip->worldTooltipRequiresButtonHeld = 1;
		}
	}
}

bool Entity::bEntityTooltipRequiresButtonHeld() const
{
	if ( behavior == &actPortal || behavior == &actLadder
		|| behavior == &::actMidGamePortal || behavior == &::actExpansionEndGamePortal
		|| behavior == &actWinningPortal || behavior == &actCustomPortal )
	{
		return true;
	}
	return false;
}

bool Entity::bEntityHighlightedForPlayer(const int player) const
{
	if ( player < 0 || player >= MAXPLAYERS )
	{
		return false;
	}
	if ( (behavior == &actPlayer || behavior == &actPlayerLimb) 
		&& StatueManager.activeEditing && highlightForUI > 0.001 )
	{
		return true;
	}
	if ( (behavior == &actMonster && !isInertMimic())
		|| behavior == &actPlayer )
	{
		return false;
	}
	if ( players[player]->worldUI.uidForActiveTooltip != 0 )
	{
		if ( players[player]->worldUI.uidForActiveTooltip == getUID() )
		{
			return true;
		}
	}
	return false;
}

int getEntityHungerInterval(int player, Entity* my, Stat* myStats, EntityHungerIntervals hungerInterval)
{
	bool isInsectoidPlayer = false;
	bool isAutomatonPlayer = false;
	if ( player >= 0 )
	{
		if ( stats[player]->type == AUTOMATON )
		{
			isAutomatonPlayer = true;
		}
		else if ( stats[player]->playerRace == RACE_INSECTOID && stats[player]->stat_appearance == 0 )
		{
			isInsectoidPlayer = true;
		}
	}
	else if ( my && my->behavior == &actPlayer && myStats )
	{
		if ( myStats->type == AUTOMATON )
		{
			isAutomatonPlayer = true;
		}
		else if ( myStats->playerRace == RACE_INSECTOID && myStats->stat_appearance == 0 )
		{
			isInsectoidPlayer = true;
		}
	}
	else if ( myStats )
	{
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			if ( myStats == stats[i] )
			{
				if ( myStats->type == AUTOMATON )
				{
					isAutomatonPlayer = true;
				}
				else if ( myStats->playerRace == RACE_INSECTOID && myStats->stat_appearance == 0 )
				{
					isInsectoidPlayer = true;
				}
				break;
			}
		}
	}

	if ( isAutomatonPlayer )
	{
		switch ( hungerInterval )
		{
			case HUNGER_INTERVAL_OVERSATIATED:
				return 5000; // unreachable
			case HUNGER_INTERVAL_HUNGRY:
				return -1; // unreachable
			case HUNGER_INTERVAL_WEAK:
				return -1; // unreachable
			case HUNGER_INTERVAL_STARVING:
				return -1; // unreachable
			case HUNGER_INTERVAL_AUTOMATON_CRITICAL:
				return 300;
			case HUNGER_INTERVAL_AUTOMATON_SUPERHEATED:
				return 1200;
			default:
				return 1000;
		}
	}

	switch ( hungerInterval )
	{
		case HUNGER_INTERVAL_OVERSATIATED:
			return 1500;
		case HUNGER_INTERVAL_HUNGRY:
			return !isInsectoidPlayer ? 250 : 100;
		case HUNGER_INTERVAL_WEAK:
			return !isInsectoidPlayer ? 150 : 50;
		case HUNGER_INTERVAL_STARVING:
			return !isInsectoidPlayer ? 50 : 25;
		default:
			break;
	}
	return 1000;
}

void Entity::alertAlliesOnBeingHit(Entity* attacker, std::unordered_set<Entity*>* skipEntitiesToAlert)
{
	if ( !attacker ) 
	{ 
		return;
	}
	Stat* hitstats = getStats();
	if ( !hitstats )
	{
		return;
	}

	bool infightingStop = false;
	static ConsoleVariable<bool> cvar_infightingprotect("/infighting_protect", true);
	static ConsoleVariable<bool> cvar_infightingprotectplayerallies("/infighting_protect_player_allies", true);
	if ( (behavior == &actMonster && attacker->behavior == &actPlayer)
		|| (behavior == &actPlayer && attacker->behavior == &actMonster) )
	{
		infightingStop = *cvar_infightingprotectplayerallies;
	}
	else if ( behavior == &actMonster && attacker->behavior == &actMonster )
	{
		if ( !monsterAllyGetPlayerLeader() && !attacker->monsterAllyGetPlayerLeader() )
		{
			infightingStop = *cvar_infightingprotect;
		}
	}

	// alert other monsters too
	Entity* ohitentity = hit.entity;
	for ( node_t* node = map.creatures->first; node != nullptr; node = node->next ) //Only searching for monsters, so don't iterate full map.entities.
	{
		Entity* entity = (Entity*)node->element;
		if ( !entity ) { continue; }
		if ( skipEntitiesToAlert && (skipEntitiesToAlert->find(entity) != skipEntitiesToAlert->end()) )
		{
			continue;
		}
		if ( entity->behavior == &actMonster && entity != this )
		{
			Stat* buddystats = entity->getStats();
			if ( buddystats != nullptr )
			{
				if ( buddystats->type == SHOPKEEPER && hitstats->type != SHOPKEEPER )
				{
					continue; // shopkeepers don't care about hitting humans/robots etc.
				}
				if ( hitstats->type == SHOPKEEPER && entity->monsterAllyGetPlayerLeader() )
				{
					continue; // hitting a shopkeeper, player followers won't retaliate against player
				}
				if ( entity->checkFriend(this) )
				{
					if ( entity->monsterState == MONSTER_STATE_WAIT )
					{
						if ( infightingStop && entity->checkFriend(attacker) )
						{
							if ( attacker->behavior == &actPlayer )
							{
								if ( (monsterAllyGetPlayerLeader() == nullptr) 
									!= (entity->monsterAllyGetPlayerLeader() == nullptr) )
								{
									// if the fight is between player allies, outside mobs do not interfere
									//messagePlayer(0, MESSAGE_DEBUG, "Stopped an ally infight 1.");
									continue;
								}
							}
							else if ( behavior == &actPlayer )
							{
								if ( (attacker->monsterAllyGetPlayerLeader() == nullptr)
									!= (entity->monsterAllyGetPlayerLeader() == nullptr) )
								{
									// if the fight is between player allies, outside mobs do not interfere
									//messagePlayer(0, MESSAGE_DEBUG, "Stopped an ally infight 2.");
									continue;
								}
							}
							else
							{
								//messagePlayer(0, MESSAGE_DEBUG, "Stopped an infight.");
								continue;
							}
						}

						real_t tangent = atan2(entity->y - this->y, entity->x - this->x);
						if ( buddystats->type == BAT_SMALL && entity->isUntargetableBat() && entity->bodyparts.size() > 0 && entity->monsterSpecialState == BAT_REST )
						{
							real_t oldZ = entity->bodyparts[0]->z;
							entity->bodyparts[0]->z = 0.0; // hack to make it linetraceable
							lineTrace(this, this->x, this->y, tangent, 64.0, 0, false);
							entity->bodyparts[0]->z = oldZ;
						}
						else
						{
							lineTrace(this, this->x, this->y, tangent, 1024, 0, false);
						}
						if ( hit.entity == entity )
						{
							if ( buddystats->type == BAT_SMALL )
							{
								if ( entity->monsterSpecialState == BAT_REST )
								{
									entity->disturbBat(attacker, true, false);
									continue;
								}
							}
							entity->monsterAcquireAttackTarget(*attacker, MONSTER_STATE_PATH);
						}
					}
				}
			}
		}
	}
	hit.entity = ohitentity;
}

void Entity::seedEntityRNG(Uint32 seed)
{
	if ( !entity_rng )
	{
		entity_rng = new BaronyRNG();
	}
	if ( entity_rng )
	{
		entity_rng->seedBytes(&seed, sizeof(seed));
	}
}

bool Entity::entityCanVomit() const
{
	if ( behavior != &actMonster && behavior != actPlayer )
	{
		return false;
	}

	Stat* myStats = getStats();
	if ( !myStats )
	{
		return false;
	}

	bool shapeshifted = false;
	if ( behavior == &actPlayer )
	{
		if ( myStats->type != HUMAN )
		{
			if ( effectShapeshift != NOTHING )
			{
				return false;
			}
		}
	}

	if ( myStats->type == SKELETON || myStats->type == AUTOMATON )
	{
		return false;
	}

	if ( !shapeshifted && myStats->mask && myStats->mask->type == MASK_PLAGUE )
	{
		return false;
	}

	if ( myStats->EFFECTS[EFF_NAUSEA_PROTECTION] )
	{
		return false;
	}

	return true;
}

int Entity::getFollowerBonusDamageResist()
{
	int resist = 0;
	if ( Stat* myStats = getStats() )
	{
		if ( behavior == &actMonster )
		{
			Entity* leader = monsterAllyGetPlayerLeader();
			if ( !leader )
			{
				if ( myStats->leader_uid != 0 )
				{
					leader = uidToEntity(myStats->leader_uid);
				}
			}
			if ( leader )
			{
				if ( Stat* stat = leader->getStats() )
				{
					if ( stat->helmet &&
						(stat->helmet->type == HAT_CROWNED_HELM) )
					{
						if ( stat->helmet->beatitude >= 0 || shouldInvertEquipmentBeatitude(stat) )
						{
							resist = std::min(50, 20 + abs(stat->helmet->beatitude) * 10);
						}
						else
						{
							resist = std::max(-50, -20 -(std::max(0, abs(stat->helmet->beatitude) - 1) * 10));
						}
					}
				}
			}
		}
	}
	return resist;
}

int Entity::getHPRestoreOnLevelUp()
{
	int hpMod = HP_MOD;

	if ( Stat* myStats = getStats() )
	{
		if ( myStats->helmet && myStats->helmet->type == HAT_CROWN )
		{
			if ( myStats->helmet->beatitude >= 0 || shouldInvertEquipmentBeatitude(myStats) )
			{
				hpMod += (std::min(50, (20 + 10 * (abs(myStats->helmet->beatitude)))) / 100.0) * myStats->MAXHP;
			}
			else
			{
				hpMod = 0;
			}
		}
		if ( behavior == &actMonster )
		{
			Entity* leader = monsterAllyGetPlayerLeader();
			if ( !leader )
			{
				if ( myStats->leader_uid != 0 )
				{
					leader = uidToEntity(myStats->leader_uid);
				}
			}
			if ( leader )
			{
				if ( Stat* stat = leader->getStats() )
				{
					if ( stat->helmet &&
						(stat->helmet->type == HAT_CROWN) )
					{
						if ( stat->helmet->beatitude >= 0 || shouldInvertEquipmentBeatitude(stat) )
						{
							hpMod += 20 + (10 * (abs(stat->helmet->beatitude)) / 100.0) * myStats->MAXHP;
						}
						else
						{
							hpMod = 0;
						}
					}
				}
			}
		}
	}
	return hpMod;
}

int Entity::getFollowerBonusHPRegen()
{
	int regen = 0;
	if ( Stat* myStats = getStats() )
	{
		if ( behavior == &actMonster )
		{
			Entity* leader = monsterAllyGetPlayerLeader();
			if ( !leader )
			{
				if ( myStats->leader_uid != 0 )
				{
					leader = uidToEntity(myStats->leader_uid);
				}
			}
			if ( leader )
			{
				if ( Stat* stat = leader->getStats() )
				{
					if ( stat->helmet &&
						(stat->helmet->type == HAT_LAURELS) )
					{
						if ( stat->helmet->beatitude >= 0 || shouldInvertEquipmentBeatitude(stat) )
						{
							regen = 1 + abs(stat->helmet->beatitude) * 1;
						}
						else
						{
							regen = -(abs(stat->helmet->beatitude) * 1);
						}
					}
				}
			}
		}
	}
	return regen;
}

int Entity::getFollowerBonusTrapResist()
{
	int resist = 0;
	if ( Stat* myStats = getStats() )
	{
		if ( behavior == &actMonster )
		{
			Entity* leader = monsterAllyGetPlayerLeader();
			if ( !leader )
			{
				if ( myStats->leader_uid != 0 )
				{
					leader = uidToEntity(myStats->leader_uid);
				}
			}
			if ( leader )
			{
				if ( Stat* stat = leader->getStats() )
				{
					if ( stat->helmet &&
						(stat->helmet->type == HAT_TURBAN) )
					{
						if ( stat->helmet->beatitude >= 0 || shouldInvertEquipmentBeatitude(stat) )
						{
							resist = std::min(100, 50 + abs(stat->helmet->beatitude) * 25);
						}
						else
						{
							resist = std::max(-100, -(abs(stat->helmet->beatitude) * 25));
						}
					}
				}
			}
		}
	}
	return resist;
}

int Entity::getEntityInspirationFromAllies()
{
	int inspiration = 0;
	if ( Stat* myStats = getStats() )
	{
		if ( behavior == &actMonster )
		{
			if ( Entity* leader = monsterAllyGetPlayerLeader() )
			{
				if ( Stat* stat = leader->getStats() )
				{
					if ( stat->helmet &&
						(stat->helmet->type == HAT_LAURELS
							|| stat->helmet->type == HAT_TURBAN
							|| stat->helmet->type == HAT_CROWN) )
					{
						if ( stat->LVL >= myStats->LVL )
						{
							if ( stat->helmet->beatitude >= 0 )
							{
								inspiration = std::min(300, 25 + (stat->helmet->beatitude * 25));
							}
							else if ( shouldInvertEquipmentBeatitude(stat) )
							{
								inspiration = std::min(300, 25 + (abs(stat->helmet->beatitude) * 25));
							}
							else
							{
								inspiration = 25;
							}
						}
					}
				}
			}
		}
		else if ( behavior == &actPlayer )
		{
			for ( int i = 0; i < MAXPLAYERS; ++i )
			{
				if ( players[i] && players[i]->entity && players[i]->entity != this )
				{
					if ( Stat* stat = stats[i] )
					{
						if ( stat->helmet &&
							(stat->helmet->type == HAT_LAURELS
								|| stat->helmet->type == HAT_TURBAN
								|| stat->helmet->type == HAT_CROWN) )
						{
							if ( stat->LVL >= myStats->LVL )
							{
								if ( stat->helmet->beatitude >= 0 )
								{
									inspiration = std::min(300, 25 + (stat->helmet->beatitude * 25));
								}
								else if ( shouldInvertEquipmentBeatitude(stat) )
								{
									inspiration = std::min(300, 25 + (abs(stat->helmet->beatitude) * 25));
								}
								else
								{
									inspiration = 25;
								}
								break;
							}
						}
					}
				}
			}
			if ( !inspiration )
			{
				for ( node_t* node = stats[this->skill[2]]->FOLLOWERS.first; node != nullptr; node = node->next )
				{
					Entity* follower = nullptr;
					if ( (Uint32*)node->element )
					{
						if ( follower = uidToEntity(*((Uint32*)node->element)) )
						{
							if ( Stat* stat = follower->getStats() )
							{
								if ( stat->helmet &&
									(stat->helmet->type == HAT_LAURELS
										|| stat->helmet->type == HAT_TURBAN
										|| stat->helmet->type == HAT_CROWN) )
								{
									if ( stat->LVL >= myStats->LVL )
									{
										if ( stat->helmet->beatitude >= 0 )
										{
											inspiration = std::min(300, 25 + (stat->helmet->beatitude * 25));
										}
										else if ( shouldInvertEquipmentBeatitude(stat) )
										{
											inspiration = std::min(300, 25 + (abs(stat->helmet->beatitude) * 25));
										}
										else
										{
											inspiration = 25;
										}
										break;
									}
								}
							}
						}
					}
				}
			}
		}
		return std::max(0, inspiration);
	}
	return 0;
}

bool Entity::doSilkenBowOnAttack(Entity* attacker)
{
	Stat* hitstats = getStats();
	if ( !hitstats || !attacker || !(attacker->behavior == &actMonster || attacker->behavior == &actPlayer) ) { return false; }

	if ( attacker == this ) { return false; }

	Stat* attackerStats = attacker->getStats();
	if ( !attackerStats ) { return false; }

	int attackerPlayer = (attacker->behavior == &actPlayer) ? attacker->skill[2] : -1;
	int playerHit = this->behavior == &actPlayer ? this->skill[2] : -1;

	bool tryEffect = false;
	if ( hitstats->helmet && hitstats->helmet->type == HAT_SILKEN_BOW )
	{
		int roll = local_rng.rand() % 100;
		int chance = 0;

		if ( hitstats->helmet->beatitude >= 0 || shouldInvertEquipmentBeatitude(hitstats) )
		{
			chance = 3 + 1 * std::min(5, abs(hitstats->helmet->beatitude));
			chance += std::min(10, (hitstats->getModifiedProficiency(PRO_LEADERSHIP)
				+ std::max(0, 3 * statGetCHR(hitstats, this))) / 10);
			chance = std::min(15, chance);
		}
		else
		{
			chance = 1;
		}

		if ( roll < chance )
		{
			tryEffect = true;
		}
	}

	if ( tryEffect )
	{
		int difficulty = getCharmMonsterDifficulty(*attacker, *attackerStats);

		int chance = 80;
		chance -= difficulty * 30;

		// special cases:
		if ( (attackerStats->type == VAMPIRE && MonsterData_t::nameMatchesSpecialNPCName(*attackerStats, "bram kindly"))
			|| (attackerStats->type == COCKATRICE && !strncmp(map.name, "Cockatrice Lair", 15))
			)
		{
			chance = 0;
		}
		else if ( attacker->behavior == &actMonster
			&& (attacker->monsterAllySummonRank != 0
				|| (attackerStats->type == INCUBUS && !strncmp(attackerStats->name, "inner demon", strlen("inner demon"))))
			)
		{
			chance = 0; // not allowed to control summons
		}

		bool pacify = true;// (local_rng.rand() % 20 == 0);

		if ( chance <= 0 )
		{
			// no effect.
			//playSoundEntity(hit.entity, 163, 64); // FailedSpell1V1.ogg
			if ( attackerPlayer >= 0 )
			{
				Uint32 color = makeColorRGB(0, 255, 0);
				messagePlayerColor(attackerPlayer, MESSAGE_COMBAT, color, Language::get(3141));
			}
			if ( playerHit >= 0 )
			{
				Uint32 color = makeColorRGB(255, 0, 0);
				messagePlayerMonsterEvent(playerHit, color, *attackerStats, Language::get(3142), Language::get(3143), MSG_COMBAT);
			}
			return false;
		}

		// loses will to attack.
		int duration = TICKS_PER_SECOND;
		if ( attacker->setEffect(EFF_PACIFY, true, duration, true) )
		{
			playSoundEntity(attacker, 168, 128); // Healing.ogg
			if ( attackerPlayer >= 0 )
			{
				Uint32 color = makeColorRGB(255, 0, 0);
				messagePlayerColor(attackerPlayer, MESSAGE_COMBAT, color, Language::get(3144));
			}
			if ( playerHit >= 0 )
			{
				Uint32 color = makeColorRGB(0, 255, 0);
				messagePlayerMonsterEvent(playerHit, color, *attackerStats, Language::get(3139), Language::get(3140), MSG_COMBAT);
			}
			spawnMagicEffectParticles(attacker->x, attacker->y, attacker->z, 685);
			return true;
		}
		else
		{
			// resists the charm.
			//playSoundEntity(attacker, 163, 64); // FailedSpell1V1.ogg
			if ( attackerPlayer >= 0 )
			{
				Uint32 color = makeColorRGB(0, 255, 0);
				messagePlayerColor(attackerPlayer, MESSAGE_COMBAT, color, Language::get(3141));
			}
			if ( playerHit >= 0 )
			{
				Uint32 color = makeColorRGB(255, 0, 0);
				messagePlayerMonsterEvent(playerHit, color, *attackerStats, Language::get(3142), Language::get(3143), MSG_COMBAT);
			}
			return false;
		}
	}

	return false;
}
