/*-------------------------------------------------------------------------------

BARONY
File: entity_shared.cpp
Desc: functions to be shared between editor.exe and barony.exe

Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
See LICENSE for details.

-------------------------------------------------------------------------------*/


#include "entity.hpp"
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
	creatureWindDir(fskill[15]),
	creatureWindVelocity(fskill[16]),
	creatureHoverZ(fskill[17]),
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
	doorUnlockWhenPowered(skill[16]),
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
	floorDecorationDestroyIfNoWall(skill[6]),
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
	actmagicVelXStore(fskill[8]),
	actmagicVelYStore(fskill[9]),
	actmagicVelZStore(fskill[10]),
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
	actmagicDelayMove(skill[30]),
	actmagicNoHitMessage(skill[31]),
	actmagicNoParticle(skill[32]),
	actmagicNoLight(skill[33]),
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
	lightSourceRGB(skill[11]),
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
	wallLockState(skill[0]),
	wallLockInvertPower(skill[1]),
	wallLockTurnable(skill[3]),
	wallLockMaterial(skill[4]),
	wallLockDir(skill[5]),
	wallLockClientInteractDelay(skill[6]),
	wallLockPlayerInteracting(skill[7]),
	wallLockPower(skill[8]),
	wallLockInit(skill[9]),
	wallLockTimer(skill[10]),
	wallLockPickable(skill[11]),
	wallLockPickHealth(skill[12]),
	wallLockPickableSkeletonKey(skill[13]),
	wallLockPreventLockpickExploit(skill[14]),
	wallLockAutoGenKey(skill[15]),
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
#ifndef EDITOR
	lerpCurrentState.resetMovement();
	lerpCurrentState.resetPosition();
	lerpPreviousState.resetMovement();
	lerpPreviousState.resetPosition();
	lerpRenderState.resetMovement();
	lerpRenderState.resetPosition();
#endif
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

Monster editorSpriteTypeToMonster(Sint32 sprite)
{
	Monster monsterType = NOTHING;
	switch ( sprite )
	{
	case 27: monsterType = HUMAN; break;
	case 30: monsterType = TROLL; break;
	case 35: monsterType = SHOPKEEPER; break;
	case 36: monsterType = GOBLIN; break;
	case 48: monsterType = SPIDER; break;
	case 62: monsterType = LICH; break;
	case 70: monsterType = GNOME; break;
	case 71: monsterType = DEVIL; break;
	case 75: monsterType = DEMON; break;
	case 76: monsterType = CREATURE_IMP; break;
	case 77: monsterType = MINOTAUR; break;
	case 78: monsterType = SCORPION; break;
	case 79: monsterType = SLIME; break;
	case 193: monsterType = SLIME; break;
	case 194: monsterType = SLIME; break;
	case 195: monsterType = SLIME; break;
	case 196: monsterType = SLIME; break;
	case 197: monsterType = SLIME; break;
	case 80: monsterType = SUCCUBUS; break;
	case 81: monsterType = RAT; break;
	case 82: monsterType = GHOUL; break;
	case 83: monsterType = SKELETON; break;
	case 84: monsterType = KOBOLD; break;
	case 85: monsterType = SCARAB; break;
	case 86: monsterType = CRYSTALGOLEM; break;
	case 87: monsterType = INCUBUS; break;
	case 88: monsterType = VAMPIRE; break;
	case 89: monsterType = SHADOW; break;
	case 90: monsterType = COCKATRICE; break;
	case 91: monsterType = INSECTOID; break;
	case 92: monsterType = GOATMAN; break;
	case 93: monsterType = AUTOMATON; break;
	case 94: monsterType = LICH_ICE; break;
	case 95: monsterType = LICH_FIRE; break;
	case 163: monsterType = SENTRYBOT; break;
	case 164: monsterType = SPELLBOT; break;
	case 165: monsterType = DUMMYBOT; break;
	case 166: monsterType = GYROBOT; break;
	case 188: monsterType = BAT_SMALL; break;
	case 189: monsterType = BUGBEAR; break;
	case 204: monsterType = MONSTER_D; break;
	case 205: monsterType = MONSTER_M; break;
	case 206: monsterType = MONSTER_S; break;
	case 207: monsterType = MONSTER_G; break;
	case 246: monsterType = REVENANT_SKULL; break;
	case 247: monsterType = MONSTER_ADORCISED_WEAPON; break;
	default:
		break;
	}
	return monsterType;
}

int checkSpriteType(Sint32 sprite)
{
	switch ( sprite )
	{
	case 71:
	case 70:
	case 62:
	case 48:
	case 36:
	case 35:
	case 30:
	case 27:
	case 10:
	case 83:
	case 84:
	case 85:
	case 86:
	case 87:
	case 88:
	case 89:
	case 90:
	case 91:
	case 92:
	case 93:
	case 94:
	case 95:
	case 75:
	case 76:
	case 77:
	// to test case 37
	case 37:
	case 78:
	case 79:
	case 80:
	case 81:
	case 82:
	case 163:
	case 164:
	case 165:
	case 166:
	case 188:
	case 189:
	case 193:
	case 194:
	case 195:
	case 196:
	case 197:
	case 204:
	case 205:
	case 206:
	case 207:
	case 246:
	case 247:
		//monsters
		return 1;
		break;
	case 21:
		//chest
		return 2;
		break;
	case 8:
		//items
		return 3;
		break;
	case 97:
		//summon trap
		return 4;
		break;
	case 106:
		//power crystal
		return 5;
		break;
	case 115:
		// lever timer
		return 6;
	case 102:
	case 103:
	case 104:
	case 105:
		//boulder traps
		return 7;
		break;
	case 116:
		//pedestal
		return 8;
		break;
	case 118:
		//teleporter
		return 9;
		break;
	case 119:
		//ceiling tile model
		return 10;
		break;
	case 120:
		//magic ceiling trap
		return 11;
		break;
	case 121:
	case 122:
	case 123:
	case 124:
	case 125:
	case 60:
		// general furniture/misc.
		return 12;
		break;
	case 127:
		// floor decoration
		return 13;
		break;
	case 130:
		// sound source
		return 14;
	case 131:
		// light source
		return 15;
	case 132:
		// text source
		return 16;
	case 133:
		// signal modifier
		return 17;
	case 161:
		// custom exit
		return 18;
	case 59:
		// table
		return 19;
	case 162: 
		// readablebook
		return 20;
	case 2:
	case 3:
		return 21;
	case 19:
	case 20:
	case 113:
	case 114:
		return 22;
	case 1:
		return 23;
	case 169:
		// statue
		return 24;
	case 177:
		// teleport shrine
		return 25;
	case 178:
		// generic spell shrine
		return 26;
	case 179:
		return 27;
	case 185:
	case 186:
	case 187:
		// AND gate
		return 28;
	case 33:
	case 34:
		// act trap
		return 29;
	case 208:
	case 209:
	case 210:
	case 211:
		// wall locks
		return 30;
	case 212:
	case 213:
	case 214:
	case 215:
		// wall buttons
		return 31;
	case 217:
	case 218:
		// iron doors
		return 32;
	case 220:
		// wind
		return 33;
	default:
		return 0;
		break;
	}

	return 0;
}

char itemNameStrings[NUM_ITEM_STRINGS][32] =
{
	"NULL",
	"random_item",
	"wooden_shield",
	"quarterstaff",
	"bronze_sword",
	"bronze_mace",
	"bronze_axe",
	"bronze_shield",
	"sling",
	"iron_spear",
	"iron_sword",
	"iron_mace",
	"iron_axe",
	"iron_shield",
	"shortbow",
	"steel_halberd",
	"steel_sword",
	"steel_mace",
	"steel_axe",
	"steel_shield",
	"steel_shield_resistance",
	"crossbow",
	"gloves",
	"gloves_dexterity",
	"bracers",
	"bracers_constitution",
	"gauntlets",
	"gauntlets_strength",
	"cloak",
	"cloak_magicreflection",
	"cloak_invisibility",
	"cloak_protection",
	"leather_boots",
	"leather_boots_speed",
	"iron_boots",
	"iron_boots_waterwalking",
	"steel_boots",
	"steel_boots_levitation",
	"steel_boots_feather",
	"leather_breastpiece",
	"iron_breastpiece",
	"steel_breastpiece",
	"hat_phrygian",
	"hat_hood",
	"hat_wizard",
	"hat_jester",
	"leather_helm",
	"iron_helm",
	"steel_helm",
	"amulet_sexchange",
	"amulet_lifesaving",
	"amulet_waterbreathing",
	"amulet_magicreflection",
	"amulet_strangulation",
	"amulet_poisonresistance",
	"potion_water",
	"potion_booze",
	"potion_juice",
	"potion_sickness",
	"potion_confusion",
	"potion_extrahealing",
	"potion_healing",
	"potion_cureailment",
	"potion_blindness",
	"potion_restoremagic",
	"potion_invisibility",
	"potion_levitation",
	"potion_speed",
	"potion_acid",
	"potion_paralysis",
	"scroll_mail",
	"scroll_identify",
	"scroll_light",
	"scroll_blank",
	"scroll_enchantweapon",
	"scroll_enchantarmor",
	"scroll_removecurse",
	"scroll_fire",
	"scroll_food",
	"scroll_magicmapping",
	"scroll_repair",
	"scroll_destroyarmor",
	"scroll_teleportation",
	"scroll_summon",
	"magicstaff_light",
	"magicstaff_digging",
	"magicstaff_locking",
	"magicstaff_magicmissile",
	"magicstaff_opening",
	"magicstaff_slow",
	"magicstaff_cold",
	"magicstaff_fire",
	"magicstaff_lightning",
	"magicstaff_sleep",
	"ring_adornment",
	"ring_slowdigestion",
	"ring_protection",
	"ring_warning",
	"ring_strength",
	"ring_constitution",
	"ring_invisibility",
	"ring_magicresistance",
	"ring_conflict",
	"ring_levitation",
	"ring_regeneration",
	"ring_teleportation",
	"spellbook_forcebolt",
	"spellbook_magicmissile",
	"spellbook_cold",
	"spellbook_fireball",
	"spellbook_light",
	"spellbook_removecurse",
	"spellbook_lightning",
	"spellbook_identify",
	"spellbook_magicmapping",
	"spellbook_sleep",
	"spellbook_confuse",
	"spellbook_slow",
	"spellbook_opening",
	"spellbook_locking",
	"spellbook_levitation",
	"spellbook_invisibility",
	"spellbook_teleportation",
	"spellbook_healing",
	"spellbook_extrahealing",
	"spellbook_cureailment",
	"spellbook_dig",
	"gem_rock",
	"gem_luck",
	"gem_garnet",
	"gem_ruby",
	"gem_jacinth",
	"gem_amber",
	"gem_citrine",
	"gem_jade",
	"gem_emerald",
	"gem_sapphire",
	"gem_aquamarine",
	"gem_amethyst",
	"gem_fluorite",
	"gem_opal",
	"gem_diamond",
	"gem_jetstone",
	"gem_obsidian",
	"gem_glass",
	"tool_pickaxe",
	"tool_tinopener",
	"tool_mirror",
	"tool_lockpick",
	"tool_skeletonkey",
	"tool_torch",
	"tool_lantern",
	"tool_blindfold",
	"tool_towel",
	"tool_glasses",
	"tool_beartrap",
	"food_bread",
	"food_creampie",
	"food_cheese",
	"food_apple",
	"food_meat",
	"food_fish",
	"food_tin",
	"readable_book",
	"spell_item",
	"artifact_sword",
	"artifact_mace",
	"artifact_spear",
	"artifact_axe",
	"artifact_bow",
	"artifact_breastpiece",
	"artifact_helm",
	"artifact_boots",
	"artifact_cloak",
	"artifact_gloves",
	"crystal_breastpiece",
	"crystal_helm",
	"crystal_boots",
	"crystal_shield",
	"crystal_gloves",
	"vampire_doublet",
	"wizard_doublet",
	"healer_doublet",
	"mirror_shield",
	"brass_knuckles",
	"iron_knuckles",
	"spiked_gauntlets",
	"food_tomalley",
	"tool_crystalshard",
	"crystal_sword",
	"crystal_spear",
	"crystal_battleaxe",
	"crystal_mace",
	"bronze_tomahawk",
	"iron_dagger",
	"steel_chakram",
	"crystal_shuriken",
	"cloak_black",
	"magicstaff_stoneblood",
	"magicstaff_bleed",
	"magicstaff_summon",
	"tool_blindfold_focus",
	"tool_blindfold_telepathy",
	"spellbook_summon",
	"spellbook_stoneblood",
	"spellbook_bleed",
	"spellbook_reflect_magic",
	"spellbook_acid_spray",
	"spellbook_steal_weapon",
	"spellbook_drain_soul",
	"spellbook_vampiric_aura",
	"spellbook_charm",
	"potion_empty",
	"artifact_orb_blue",
	"artifact_orb_red",
	"artifact_orb_purple",
	"artifact_orb_green",
	"tunic",
	"hat_fez",
	"magicstaff_charm",
	"potion_polymorph",
	"food_blood",
	"cloak_backpack",
	"tool_alembic",
	"potion_firestorm",
	"potion_icestorm",
	"potion_thunderstorm",
	"potion_strength",
	"suede_boots",
	"suede_gloves",
	"cloak_silver",
	"hat_hood_silver",
	"hat_hood_red",
	"silver_doublet",
	"spellbook_revert_form",
	"spellbook_rat_form",
	"spellbook_spider_form",
	"spellbook_troll_form",
	"spellbook_imp_form",
	"spellbook_spray_web",
	"spellbook_poison",
	"spellbook_speed",
	"spellbook_fear",
	"spellbook_strike",
	"spellbook_detect_food",
	"spellbook_weakness",
	"mask_shaman",
	"spellbook_amplify_magic",
	"spellbook_shadow_tag",
	"spellbook_telepull",
	"spellbook_demon_illu",
	"spellbook_trolls_blood",
	"spellbook_salvage",
	"tool_whip",
	"spellbook_flutter",
	"spellbook_dash",
	"spellbook_self_polymorph",
	"spellbook_9",
	"spellbook_10",
	"magicstaff_poison",
	"tool_metal_scrap",
	"tool_magic_scrap",
	"tool_tinkering_kit",
	"tool_sentrybot",
	"tool_detonated_charge",
	"tool_fire_bomb",
	"tool_sleep_bomb",
	"tool_freeze_bomb",
	"tool_teleport_bomb",
	"tool_gyrobot",
	"tool_spellbot",
	"tool_decoy",
	"tool_dummybot",
	"machinist_apron",
	"enchanted_feather",
	"punisher_hood",
	"scroll_charging",
	"quiver_silver",
	"quiver_pierce",
	"quiver_lightweight",
	"quiver_fire",
	"quiver_heavy",
	"quiver_crystal",
	"quiver_hunting",
	"longbow",
	"compound_bow",
	"heavy_crossbow",
	"boomerang",
	"scroll_conjurearrow",
	"monocle",
	"tool_player_loot_bag",
	"mask_bandit",
	"mask_eyepatch",
	"mask_masquerade",
	"mask_mouth_rose",
	"mask_golden",
	"mask_spooky",
	"mask_tech_goggles",
	"mask_hazard_goggles",
	"mask_phantom",
	"mask_pipe",
	"mask_grass_sprig",
	"mask_plague",
	"mask_mouthknife",
	"hat_silken_bow",
	"hat_plumed_cap",
	"hat_bycocket",
	"hat_tophat",
	"hat_bandana",
	"hat_circlet",
	"hat_crown",
	"hat_laurels",
	"hat_turban",
	"hat_crowned_helm",
	"hat_warm",
	"hat_wolf_hood",
	"hat_bear_hood",
	"hat_stag_hood",
	"hat_bunny_hood",
	"hat_bountyhunter",
	"hat_miter",
	"hat_headdress",
	"hat_chef",
	"helm_mining",
	"mask_steel_visor",
	"mask_crystal_visor",
	"mask_artifact_visor",
	"hat_circlet_wisdom",
	"hat_hood_apprentice",
	"hat_hood_assassin",
	"hat_hood_whispers",
	"ring_resolve",
	"cloak_guardian",
	"mask_marigold",
	"key_stone",
	"key_bone",
	"key_bronze",
	"key_iron",
	"key_silver",
	"key_gold",
	"key_crystal",
	"key_machine",
	"tool_foci_fire",
	"instrument_flute",
	"instrument_lyre",
	"instrument_drum",
	"instrument_lute",
	"instrument_horn",
	"rapier",
	"amulet_burningresist",
	"grease_ball",
	"branch_staff",
	"branch_bow",
	"branch_bow_infected",
	"dust_ball",
	"bolas",
	"steel_flail",
	"food_ration",
	"food_ration_spicy",
	"food_ration_sour",
	"food_ration_bitter",
	"food_ration_hearty",
	"food_ration_herbal",
	"food_ration_sweet",
	"slop_ball",
	"tool_frying_pan",
	"cleat_boots",
	"bandit_breastpiece",
	"tunic_blouse",
	"bone_breastpiece",
	"blackiron_breastpiece",
	"silver_breastpiece",
	"iron_pauldrons",
	"quilted_gambeson",
	"robe_cultist",
	"robe_healer",
	"robe_monk",
	"robe_wizard",
	"shawl",
	"chain_hauberk",
	""
};

char itemStringsByType[10][NUM_ITEM_STRINGS_BY_TYPE][32] =
{
	{
		"NULL",
		"random_item",
		"hat_phrygian",
		"hat_hood",
		"hat_wizard",
		"hat_jester",
		"hat_fez",
		"hat_hood_silver",
		"hat_hood_red",
		"mask_shaman",
		"punisher_hood",
		"leather_helm",
		"iron_helm",
		"steel_helm",
		"crystal_helm",
		"artifact_helm",
		"hat_silken_bow",
		"hat_plumed_cap",
		"hat_bycocket",
		"hat_tophat",
		"hat_bandana",
		"hat_circlet",
		"hat_crown",
		"hat_laurels",
		"hat_turban",
		"hat_crowned_helm",
		"hat_warm",
		"hat_wolf_hood",
		"hat_bear_hood",
		"hat_stag_hood",
		"hat_bunny_hood",
		"hat_bountyhunter",
		"hat_miter",
		"hat_headdress",
		"hat_chef",
		"helm_mining",
		"hat_circlet_wisdom",
		"hat_hood_apprentice",
		"hat_hood_assassin",
		"hat_hood_whispers",
		""
	},
	{
		"NULL",
		"random_item",
		"quarterstaff",
		"bronze_sword",
		"bronze_mace",
		"bronze_axe",
		"sling",
		"iron_spear",
		"iron_sword",
		"iron_mace",
		"iron_axe",
		"shortbow",
		"steel_halberd",
		"steel_sword",
		"steel_mace",
		"steel_axe",
		"crystal_sword",
		"crystal_spear",
		"crystal_battleaxe",
		"crystal_mace",
		"crossbow",
		"bronze_tomahawk",
		"iron_dagger",
		"steel_chakram",
		"crystal_shuriken",
		"boomerang",
		"longbow",
		"compound_bow",
		"heavy_crossbow",
		"potion_water",
		"potion_booze",
		"potion_juice",
		"potion_sickness",
		"potion_confusion",
		"potion_extrahealing",
		"potion_healing",
		"potion_cureailment",
		"potion_blindness",
		"potion_restoremagic",
		"potion_invisibility",
		"potion_levitation",
		"potion_speed",
		"potion_acid",
		"potion_paralysis",
		"potion_polymorph",
		"potion_firestorm",
		"potion_icestorm",
		"potion_thunderstorm",
		"potion_strength",
		"magicstaff_light",
		"magicstaff_digging",
		"magicstaff_locking",
		"magicstaff_magicmissile",
		"magicstaff_opening",
		"magicstaff_slow",
		"magicstaff_cold",
		"magicstaff_fire",
		"magicstaff_lightning",
		"magicstaff_sleep",
		"magicstaff_stoneblood",
		"magicstaff_bleed",
		"magicstaff_summon",
		"magicstaff_charm",
		"magicstaff_poison",
		"spellbook_forcebolt",
		"spellbook_magicmissile",
		"spellbook_cold",
		"spellbook_fireball",
		"spellbook_light",
		"spellbook_removecurse",
		"spellbook_lightning",
		"spellbook_identify",
		"spellbook_magicmapping",
		"spellbook_sleep",
		"spellbook_confuse",
		"spellbook_slow",
		"spellbook_opening",
		"spellbook_locking",
		"spellbook_levitation",
		"spellbook_invisibility",
		"spellbook_teleportation",
		"spellbook_healing",
		"spellbook_extrahealing",
		"spellbook_cureailment",
		"spellbook_summon",
		"spellbook_stoneblood",
		"spellbook_bleed",
		"spellbook_dig",
		"spellbook_reflect_magic",
		"spellbook_acid_spray",
		"spellbook_steal_weapon",
		"spellbook_drain_soul",
		"spellbook_vampiric_aura",
		"spellbook_charm",
		"spellbook_revert_form",
		"spellbook_rat_form",
		"spellbook_spider_form",
		"spellbook_troll_form",
		"spellbook_imp_form",
		"spellbook_spray_web",
		"spellbook_poison",
		"spellbook_speed",
		"spellbook_fear",
		"spellbook_strike",
		"spellbook_detect_food",
		"spellbook_weakness",
		"spellbook_amplify_magic",
		"spellbook_shadow_tag",
		"spellbook_telepull",
		"spellbook_demon_illu",
		"spellbook_trolls_blood",
		"spellbook_salvage",
		"spellbook_flutter",
		"spellbook_dash",
		"spellbook_self_polymorph",
		"spellbook_9",
		"spellbook_10",
		"tool_whip",
		"tool_pickaxe",
		"artifact_sword",
		"artifact_mace",
		"artifact_spear",
		"artifact_axe",
		"artifact_bow",
		"artifact_orb_blue",
		"artifact_orb_red",
		"artifact_orb_purple",
		"artifact_orb_green",
		"rapier",
		"grease_ball",
		"branch_staff",
		"branch_bow",
		"branch_bow_infected",
		"dust_ball",
		"bolas",
		"steel_flail",
		"slop_ball",
		""
	},
	{
		"NULL",
		"random_item",
		"wooden_shield",
		"bronze_shield",
		"iron_shield",
		"steel_shield",
		"steel_shield_resistance",
		"crystal_shield",
		"mirror_shield",
		"tool_torch",
		"tool_lantern",
		"tool_crystalshard",
		"quiver_silver",
		"quiver_pierce",
		"quiver_lightweight",
		"quiver_fire",
		"quiver_heavy",
		"quiver_crystal",
		"quiver_hunting",
		"tool_foci_fire",
		"instrument_flute",
		"instrument_lyre",
		"instrument_drum",
		"instrument_lute",
		"instrument_horn",
		"tool_frying_pan",
		""
	},
	{
		"NULL",
		"random_item",
		"leather_breastpiece",
		"iron_breastpiece",
		"steel_breastpiece",
		"crystal_breastpiece",
		"artifact_breastpiece",
		"vampire_doublet",
		"wizard_doublet",
		"healer_doublet",
		"tunic",
		"silver_doublet",
		"machinist_apron",
		"bandit_breastpiece",
		"tunic_blouse",
		"bone_breastpiece",
		"blackiron_breastpiece",
		"silver_breastpiece",
		"iron_pauldrons",
		"quilted_gambeson",
		"robe_cultist",
		"robe_healer",
		"robe_monk",
		"robe_wizard",
		"shawl",
		"chain_hauberk",
		""
	},
	{
		"NULL",
		"random_item",
		"leather_boots",
		"leather_boots_speed",
		"iron_boots",
		"iron_boots_waterwalking",
		"steel_boots",
		"steel_boots_levitation",
		"steel_boots_feather",
		"crystal_boots",
		"artifact_boots",
		"suede_boots",
		"cleat_boots",
		""
	},
	{
		"NULL",
		"random_item",
		"ring_adornment",
		"ring_slowdigestion",
		"ring_protection",
		"ring_warning",
		"ring_strength",
		"ring_constitution",
		"ring_invisibility",
		"ring_magicresistance",
		"ring_conflict",
		"ring_levitation",
		"ring_regeneration",
		"ring_teleportation",
		"ring_resolve",
		""
	},
	{
		"NULL",
		"random_item",
		"amulet_sexchange",
		"amulet_lifesaving",
		"amulet_waterbreathing",
		"amulet_magicreflection",
		"amulet_strangulation",
		"amulet_poisonresistance",
		"amulet_burningresist",
		""
	},
	{
		"NULL",
		"random_item",
		"cloak",
		"cloak_black",
		"cloak_silver",
		"cloak_magicreflection",
		"cloak_invisibility",
		"cloak_protection",
		"artifact_cloak",
		"cloak_backpack",
		"cloak_guardian",
		""
	},
	{
		"NULL",
		"random_item",
		"tool_blindfold",
		"tool_glasses",
		"tool_blindfold_focus",
		"tool_blindfold_telepathy",
		"monocle",
		"mask_bandit",
		"mask_eyepatch",
		"mask_masquerade",
		"mask_mouth_rose",
		"mask_golden",
		"mask_spooky",
		"mask_tech_goggles",
		"mask_hazard_goggles",
		"mask_phantom",
		"mask_pipe",
		"mask_grass_sprig",
		"mask_plague",
		"mask_mouthknife",
		"mask_steel_visor",
		"mask_crystal_visor",
		"mask_artifact_visor",
		"mask_marigold",
		""
	},
	{
		"NULL",
		"random_item",
		"gloves",
		"gloves_dexterity",
		"bracers",
		"bracers_constitution",
		"gauntlets",
		"gauntlets_strength",
		"crystal_gloves",
		"artifact_gloves",
		"brass_knuckles",
		"iron_knuckles",
		"spiked_gauntlets",
		"suede_gloves",
		""
	}
	
};

std::vector<const char*> spriteEditorNameStrings =
{
	"NULL",	
	"PLAYER START",
	"DOOR (East-West)",
	"DOOR (North-South)",
	"TORCH (West Wall)",
	"TORCH (North Wall)",
	"TORCH (East Wall)",
	"TORCH (South Wall)",
	"ITEM",
	"GOLD",
	"RANDOM (Dependent on Level)",
	"LADDER",
	"FIREPLACE",
	"Flame Sprite (Not Used)",
	"FOUNTAIN",
	"SINK",
	"Flame Sprite (Not Used)",
	"Lever",
	"Wire",
	"GATE (North-South)",
	"GATE (East-West)",
	"CHEST",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"HUMAN",
	"NOT USED",
	"NOT USED",
	"TROLL",
	"NOT USED",
	"ARROW TRAP",
	"PRESSURE PLATE",
	"PRESSURE PLATE (Latch On)",
	"SHOPKEEPER",
	"GOBLIN",
	"MINOTAUR SPAWN TRAP",
	"BOULDER TRAP",
	"HEADSTONE",
	"NULL",
	"LAVA",
	"NOT USED",
	"LADDER HOLE",
	"BOULDER",
	"PORTAL",
	"SECRET LADDER",
	"NOT USED",
	"SPIDER",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"TABLE",
	"CHAIR",
	"DIAMOND PICKAXE",
	"LICH",
	"END PORTAL",
	"SPEAR TRAP",
	"MAGIC TRAP",
	"WALL BUSTER",
	"WALL BUILDER",
	"MAGIC BOW",
	"MAGIC SPEAR",
	"GNOME",
	"DEVIL",
	"DEVIL TELEPORT LOCATION",
	"DEVIL TELEPORT LOCATION",
	"DEVIL TELEPORT LOCATION",
	"DEMON",
	"IMP",
	"MINOTAUR",
	"SCORPION",
	"SLIME",
	"SUCCUBUS",
	"RAT",
	"GHOUL",
	"SKELETON",
	"KOBOLD",
	"SCARAB",
	"CRYSTALGOLEM",
	"INCUBUS",
	"VAMPIRE",
	"SHADOW",
	"COCKATRICE",
	"INSECTOID",
	"GOATMAN",
	"AUTOMATON",
	"LICH ICE",
	"LICH FIRE",
	"NOT USED",
	"SUMMON TRAP",
	"CRYSTAL SHARD (West Wall)",
	"CRYSTAL SHARD (North Wall)",
	"CRYSTAL SHARD (East Wall)",
	"CRYSTAL SHARD (South Wall)",
	"BOULDER TRAP SINGLE (Roll East)",
	"BOULDER TRAP SINGLE (Roll South)",
	"BOULDER TRAP SINGLE (Roll West)",
	"BOULDER TRAP SINGLE (Roll North)",
	"POWER CRYSTAL",
	"ARMED BEAR TRAP",
	"STALAG-COLUMN",
	"STALAGMITE SINGLE",
	"STALAGMITE MULTIPLE",
	"STALAGTITE SINGLE",
	"STALAGTITE MULTIPLE",
	"GATE INVERTED (North-South)",
	"GATE INVERTED (East-West)",
	"LEVER WITH TIMER",
	"PEDESTAL",
	"MID PORTAL",
	"TELEPORTER",
	"CEILING TILE MODEL",
	"SPELL TRAP CEILING",
	"ARCANE CHAIR",
	"ARCANE BED",
	"BUNK BED",
	"COLUMN DECO",
	"PODIUM",
	"PISTONS",
	"DECORATION",
	"TELEPORT LOCATION",
	"ENDEND PORTAL",
	"SOUND SOURCE",
	"LIGHT SOURCE",
	"TEXT SOURCE",
	"SIGNAL TIMER",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"CUSTOM EXIT",
	"READABLE BOOK",
	"SENTRYBOT",
	"SPELLBOT",
	"DUMMYBOT",
	"GYROBOT",
	"UNUSED",
	"STATUE ANIMATOR",
	"STATUE",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"TELEPORT SHRINE",
	"SPELL SHRINE",
	"COLLIDER DECORATION",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"AND GATE",
	"AND GATE",
	"AND GATE",
	"BAT",
	"BUGBEAR",
	"DAEDALUS SHRINE",
	"BELL",
	"NOT USED",
	"SLIME (GREEN)",
	"SLIME (BLUE)",
	"SLIME (RED)",
	"SLIME (TAR)",
	"SLIME (METAL)",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"ASSIST SHRINE",
	"NOT USED",
	"NOT USED",
	"MONSTER_D",
	"MONSTER_M",
	"MONSTER_S",
	"MONSTER_G",
	"WALL LOCK (East)",
	"WALL LOCK (South)",
	"WALL LOCK (West)",
	"WALL LOCK (North)",
	"WALL BUTTON (East)",
	"WALL BUTTON (South)",
	"WALL BUTTON (West)",
	"WALL BUTTON (North)",
	"NO DIG TILE",
	"IRON DOOR (North-South)",
	"IRON DOOR (East-West)",
	"SLIPPERY TILE",
	"WIND TILE",
	"SLOW TILE",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"REVENANT_SKULL",
	"ADORCISED_WEAPON"
};

char monsterEditorNameStrings[NUMMONSTERS][32] =
{
	"nothing",
	"human",
	"rat",
	"goblin",
	"slime",
	"troll",
	"bat",
	"spider",
	"ghoul",
	"skeleton",
	"scorpion",
	"imp",
	"invalid",
	"gnome",
	"demon",
	"succubus",
	"mimic",
	"lich",
	"minotaur",
	"devil",
	"shopkeeper",
	"kobold",
	"scarab",
	"crystalgolem",
	"incubus",
	"vampire",
	"shadow",
	"cockatrice",
	"insectoid",
	"goatman",
	"automaton",
	"lich_ice",
	"lich_fire",
	"sentrybot",
	"spellbot",
	"gyrobot",
	"dummybot",
	"bugbear",
	"monster_d",
	"monster_m",
	"monster_s",
	"monster_g",
	"revenant_skull",
	"minimimic",
	"monster_adorcised_weapon",
	"flame_elemental",
	"hologram",
	"moth",
	"earth_elemental",
	"monster_unused_5",
	"monster_unused_6",
	"monster_unused_7",
	"monster_unused_8"
};

char tileEditorNameStrings[NUM_EDITOR_TILES][44] =
{
	"backdrop.png",
	"bback.png",
	"bbrick.png",
	"greenbrick.png",
	"graysquare.png",
	"sand.png",
	"rock.png",
	"arrow.png",
	" Smooth Stone Wall.png",
	" Wood.png",
	" Cobblestone Mine Wall.png",
	" Cobblestone Mine Support Wall.png",
	"Gray Brick.png",
	"Dirt.png",
	"Grass.png",
	"Green Cobblestone.png",
	"Red Diamond Tiles.png",
	"Blue Pillar.png",
	"Tan Brick Wall.png",
	"Mossy Tan Brick Wall.png",
	"Red Cobblestone Mine Wall.png",
	"Red Cobblestone Mine Support Wall.png",
	"water1.png",
	"water2.png",
	"water3.png",
	"water4.png",
	"water5.png",
	"water6.png",
	"water7.png",
	"water8.png",
	"Plank Ceiling.png",
	"Plankand Rafter Ceiling.png",
	"Fancy Brick Wall.png",
	"Fancy Gray Brick Wall.png",
	"sandfloor.png",
	"sandfloordark.png",
	"graytiles.png",
	"Roots.png",
	"greenbrick Crack.png",
	"Green Cobblestone Vine.png",
	"Green Cobblestone No Grass.png",
	"rd A.png",
	"rd B.png",
	"rd C.png",
	"rd D.png",
	"rd E.png",
	"rd F.png",
	"rd G.png",
	"rd H.png",
	"rd I.png",
	"Hard Stone.png",
	"Plankand Rafter Ceiling Horiz.png",
	"Red Square Tiles.png",
	"Trap Wall.png",
	"shopsign.png",
	"Shelf.png",
	"Dirt Two.png",
	"Purple Square Tiles.png",
	"Grass Two.png",
	"Purple Diamond Tiles.png",
	"Dull Green Brick.png",
	"Dull Green Brick Skull.png",
	"Dull Green Square.png",
	"Gray Dirt.png",
	"Lava1.png",
	"Lava2.png",
	"Lava3.png",
	"Lava4.png",
	"Lava5.png",
	"Lava6.png",
	"Lava7.png",
	"Lava8.png",
	"Big Orange Stone.png",
	"Red Pillar.png",
	"Red Brick.png",
	"Red Square.png",
	"Red Brick Face.png",
	"clouds.png",
	"Magic Trap Wall1.png",
	"Magic Trap Wall2.png",
	"Magic Trap Wall3.png",
	"Magic Trap Wall4.png",
	"clouds.png",
	"Leaves.png",
	"Cobblestone.png",
	"Cobblestone Mine Support Sides.png",
	"Cobblestone Mine Support Side L.png",
	"Cobblestone Mine Support Side R.png",
	"Cobblestone Mine Wall Two.png",
	"Cobblestone Mossy.png",
	"Cobblestone Mine Support Wall Mossy.png",
	"Cobblestone Mine Support Sides Mossy.png",
	"Cobblestone Mine Wall Mossy.png",
	"Cobblestone Mine Wall Two Mossy.png",
	"Red Cobblestone Mine Wall Support Sides.png",
	"Red Cobblestone Mine Wall Support L.png",
	"Red Cobblestone Mine Wall Support R.png",
	"Wood Two.png",
	"Wood Corner NE.png",
	"Wood Corner NW.png",
	"Wood Corner SE.png",
	"Wood Corner SW.png",
	"Plankand Rafter Ceiling Cross.png",
	"Gray Brick Block.png",
	"Gray Brick Columns.png",
	"Gray Brick Column R.png",
	"Gray Brick Column L.png",
	"Gray Brick Column Center.png",
	"Swamp Logs.png",
	"Swamp Logs Two.png",
	"Swamp Log Ends.png",
	"Roots Treetop.png",
	"Swamp Log L.png",
	"Swamp Log R.png",
	"greenbricktwo.png",
	"Green Cobblestone Two.png",
	"Green Cobblestone Column.png",
	"Green Cobblestone Column No Grass.png",
	"Fancy Sandstone Wall.png",
	"Orange Square Tiles.png",
	"Ruin Tiles.png",
	"Red Brick Face1.png",
	"Red Brick Face2.png",
	"Red Brick Face3.png",
	"Red Brick Face4.png",
	"Red Brick Face5.png",
	"Red Brick Face6.png",
	"Red Brick Face7.png",
	"Hell Tiles.png",
	"Skull Lava1.png",
	"Skull Lava2.png",
	"Skull Lava3.png",
	"Skull Lava4.png",
	"Skull Lava5.png",
	"Skull Lava6.png",
	"bonewall.png",
	"Lavafall1.png",
	"Lavafall2.png",
	"Lavafall3.png",
	"bonewallwithgrass.png",
	"bonewallpillar.png",
	"decayedstone.png",
	"Dirt Path -  E.png",
	"Dirt Path -  S.png",
	"Dirt Path -  W.png",
	"Dirt Path -  N.png",
	"Dirt Path -  NE.png",
	"Dirt Path -  SE.png",
	"Dirt Path -  SW.png",
	"Dirt Path -  NW.png",
	"Dirt Path -  NE O.png",
	"Dirt Path -  SE O.png",
	"Dirt Path -  SW O.png",
	"Dirt Path -  NW O.png",
	"Cave Floor.png",
	"Cave Floor Rough.png",
	"Cave Wall.png",
	"Cave Wall Decor.png",
	"Cave Wall Crystal.png",
	"Cave Wall Reinforced.png",
	"Cave Wall Reinforced Left.png",
	"Cave Wall Reinforced Right.png",
	"Cave Wall Reinforced Center.png",
	"Cave Wall Reinforced High.png",
	"Cave Wall Alcove.png",
	"Cave Wall Columns.png",
	"Cave Wall Column Center.png",
	"Cave Wall Column Left.png",
	"Cave Wall Column Right.png",
	"Caves To Crystal Wall Left.png",
	"Caves To Crystal Wall Right.png",
	"Caves To Crystal Floor -  W.png",
	"Caves To Crystal Floor -  E.png",
	"Caves To Crystal Floor -  S.png",
	"Caves To Crystal Floor -  N.png",
	"Caves To Crystal Floor -  SW.png",
	"Caves To Crystal Floor -  NW.png",
	"Caves To Crystal Floor -  SE.png",
	"Caves To Crystal Floor -  NE.png",
	"Caves To Crystal Floor -  SW O.png",
	"Caves To Crystal Floor -  NW O.png",
	"Caves To Crystal Floor -  SE O.png",
	"Caves To Crystal Floor -  NE O.png",
	"Crystal Floor.png",
	"Crystal Floor Rough.png",
	"Crystal Wall.png",
	"Crystal Wall Decor1.png",
	"Crystal Wall Decor2.png",
	"Crystal Wall Decor3.png",
	"Crystal Wall Decor4.png",
	"Crystal Wall Reinforced.png",
	"Crystal Wall Reinforced Left.png",
	"Crystal Wall Reinforced Right.png",
	"Crystal Wall Reinforced Center.png",
	"Crystal Wall Reinforced High.png",
	"Crystal Wall Columns.png",
	"Crystal Wall Column Center.png",
	"Crystal Wall Column Left.png",
	"Crystal Wall Column Right.png",
	"Bronze Columns.png",
	"Bronze Columns Alcove.png",
	"Submap.png",
	"Cave Wall Reinforced No Beam.png",
	"Cave Wall Reinforced Left Cap.png",
	"Cave Wall Reinforced Right Cap.png",
	"Crystal Wall Reinforced No Beam.png",
	"Crystal Wall Reinforced Left Cap.png",
	"Crystal Wall Reinforced Right Cap.png",
	"Crystal Floor Trap 1.png",
	"Crystal Floor Trap 2.png",
	"Crystal Floor Trap 3.png",
	"Crystal Floor Trap 4.png",
	"Arcane Crystal H.png",
	"Arcane Crystal J.png",
	"Arcane Crystal Plating.png",
	"Arcane Crystal Tile.png",
	"Arcane Crystal V.png",
	"Arcane Panel Blue.png",
	"Arcane Panel BlueOpen.png",
	"Arcane Panel Gold.png",
	"Arcane Panel GoldOpen.png",
	"Arcane Pipes Blue.png",
	"Arcane Pipes Blue H.png",
	"Arcane Pipes Blue J.png",
	"Arcane Pipes Blue Plating.png",
	"Arcane Pipes Blue Plating Decor.png",
	"Arcane Pipes Blue V.png",
	"Arcane Pipes Gold.png",
	"Arcane Pipes Gold H.png",
	"Arcane Pipes Gold J.png",
	"Arcane Pipes Gold Plating.png",
	"Arcane Pipes Gold Plating Decor.png",
	"Arcane Pipes Gold V.png",
	"Bronze Column Pipe.png",
	"Sky.png",
	"SkyCrackle.png",
	"SkyCrackle_B.png",
	"Shopsign_armsnarmor.png", 
	"Shopsign_book.png", 
	"Shopsign_deli.png", 
	"Shopsign_hardware.png", 
	"Shopsign_hat.png", 
	"Shopsign_hunting.png", 
	"Shopsign_jewelry.png",
	"Shopsign_magicstaff.png", 
	"Shopsign_potion.png",
	"Transparent.png", 
	"Replace Me", "Replace Me", "Replace Me", "Replace Me", "Replace Me", "Replace Me", "Replace Me", "Replace Me", "Replace Me", "Replace Me",
	"Replace Me", "Replace Me", "Replace Me", "Replace Me", "Replace Me", "Replace Me", "Replace Me", "Replace Me", "Replace Me", "Replace Me",
	"Replace Me", "Replace Me", "Replace Me", "Replace Me", "Replace Me", "Replace Me", "Replace Me", "Replace Me", "Replace Me", "Replace Me",
	"Replace Me", "Replace Me", "Replace Me", "Replace Me", "Replace Me", "Replace Me", "Replace Me", "Replace Me", "Replace Me", "Replace Me",
	"Replace Me", "Replace Me", "Replace Me", "Replace Me", "Replace Me", "Replace Me", "Replace Me", "Replace Me", "Replace Me", "Replace Me",
	"Replace Me", "Replace Me", "Replace Me", "Replace Me", "Replace Me", "Replace Me", "Replace Me", "Replace Me", "Replace Me", "Replace Me",
	"Replace Me", "Replace Me", "Replace Me", "Replace Me", "Replace Me", "Replace Me", "Replace Me", "Replace Me", "Replace Me", "Replace Me",
	"Replace Me", "Replace Me", "Replace Me", "Replace Me", "Replace Me", "Replace Me", "Replace Me", "Replace Me", "Replace Me", "Replace Me",
	"Replace Me", "Replace Me", "Replace Me", "Replace Me", "Replace Me", "Replace Me", "Replace Me", "Replace Me", "Replace Me", "Replace Me",
	"Replace Me", "Replace Me", "Replace Me", "Replace Me", "Replace Me", "Replace Me", "Replace Me", "Replace Me", "Replace Me", "Replace Me",
	"Replace Me", "Replace Me", "Replace Me"
};

int canWearEquip(Entity* entity, int category)
{
	Stat* stats;
	int equipType = 0;
	int type;
	if ( entity != NULL )
	{
		stats = entity->getStats();
		if ( stats != NULL )
		{
			type = stats->type;

			switch ( type )
			{
				//monsters that don't wear equipment (only rings/amulets)
				case DEVIL:
				case SPIDER:
				case TROLL:
				case RAT:
				case SLIME:
				case SCORPION:
				case MINOTAUR:
				case GHOUL:
				case SCARAB:
				case CRYSTALGOLEM:
				case COCKATRICE:
				case MIMIC:
				case MINIMIMIC:
				case REVENANT_SKULL:
				case FLAME_ELEMENTAL:
				case EARTH_ELEMENTAL:
				case HOLOGRAM:
				case MOTH_SMALL:
					equipType = 0;
					break;

				//monsters with weapons only (incl. spellbooks)
				case LICH:
				case CREATURE_IMP:
				case DEMON:
				case MONSTER_ADORCISED_WEAPON:
					equipType = 1;
					break;

				//monsters with cloak/weapon/shield/boots/mask/gloves (no helm)
				case BUGBEAR:
				case INCUBUS:
				case SUCCUBUS:
				case LICH_FIRE:
				case LICH_ICE:
					equipType = 2;
					break;

				//monsters with cloak/weapon/shield/boots/helm/armor/mask/gloves
				case GNOME:
				case GOBLIN:
				case HUMAN:
				case VAMPIRE:
				case SKELETON:
				case SHOPKEEPER:
				case SHADOW:
				case AUTOMATON:
				case GOATMAN:
				case KOBOLD:
				case INSECTOID:
				case MONSTER_D:
				case MONSTER_M:
				case MONSTER_S:
				case MONSTER_G:
					equipType = 3;
					break;

				default:
					equipType = 0;
					break;
			}
		}
	}

	if ( category == 0 && equipType >= 3 ) //HELM
	{
		return 1;
	}
	else if ( category == 1 && equipType >= 1 ) //WEAPON
	{
		return 1;
	}
	else if ( category == 2 && equipType >= 2 ) //SHIELD
	{
		return 1;
	}
	else if ( category == 3 && equipType >= 3 ) //ARMOR
	{
		return 1;
	}
	else if ( category == 4 && equipType >= 2 ) //BOOTS
	{
		return 1;
	}
	else if ( category == 5 || category == 6 )  //RINGS/AMULETS WORN BY ALL
	{
		return 1;
	}
	else if ( (category >= 7 && category <= 9) && equipType >= 2 ) //CLOAK/MASK/GLOVES
	{
		return 1;
	}
	else
	{
		return 0;
	}

	return 0;
}

void setSpriteAttributes(Entity* entityNew, Entity* entityToCopy, Entity* entityStatToCopy)
{
	Stat* tmpStats = nullptr;
	if ( !entityNew )
	{
		return;
	}

	if ( entityStatToCopy != nullptr )
	{
		tmpStats = entityStatToCopy->getStats();
	}

	int spriteType = checkSpriteType(entityNew->sprite);
	// monsters.
	if ( spriteType == 1 )
	{
		//STAT ASSIGNMENT
		Stat* myStats = nullptr;
		if ( multiplayer != CLIENT )
		{
			// need to give the entity its list stuff.
			// create an empty first node for traversal purposes
			node_t* node2 = list_AddNodeFirst(&entityNew->children);
			node2->element = nullptr;
			node2->deconstructor = &emptyDeconstructor;

			node2 = list_AddNodeLast(&entityNew->children);
			if ( tmpStats != nullptr )
			{
				node2->element = tmpStats->copyStats();
				node2->size = sizeof(tmpStats);
			}
			else
			{
				// if the previous sprite did not have stats initialised, or creating a new entity.
				myStats = new Stat(entityNew->sprite);
				node2->element = myStats;
				node2->size = sizeof(myStats);
			}
			node2->deconstructor = &statDeconstructor;
		}
	}
	// chests.
	else if ( spriteType == 2 )
	{
		if ( entityToCopy != nullptr )
		{
			// copy old entity attributes to newly created.
			entityNew->yaw = entityToCopy->yaw;
			entityNew->skill[9] = entityToCopy->skill[9];
			entityNew->chestLocked = entityToCopy->chestLocked;
			entityNew->chestMimicChance = entityToCopy->chestMimicChance;
		}
		else
		{
			// set default new entity attributes.
			entityNew->yaw = 1;
			entityNew->skill[9] = 0;
			entityNew->chestLocked = -1;
			entityNew->chestMimicChance = -1;
		}
	}
	// items.
	else if ( spriteType == 3 )
	{
		if ( entityToCopy != nullptr )
		{
			// copy old entity attributes to newly created.
			entityNew->skill[10] = entityToCopy->skill[10];
			entityNew->skill[11] = entityToCopy->skill[11];
			entityNew->skill[12] = entityToCopy->skill[12];
			entityNew->skill[13] = entityToCopy->skill[13];
			entityNew->skill[15] = entityToCopy->skill[15];
			entityNew->skill[16] = entityToCopy->skill[16];
		}
		else
		{
			// set default new entity attributes.
			entityNew->skill[10] = 1;
			entityNew->skill[11] = 0;
			entityNew->skill[12] = 10;
			entityNew->skill[13] = 1;
			entityNew->skill[15] = 0;
			entityNew->skill[16] = 0;
		}
	}
	// summoning trap.
	else if ( spriteType == 4 )
	{
		if ( entityToCopy != nullptr )
		{
			// copy old entity attributes to newly created.
			entityNew->skill[0] = entityToCopy->skill[0];
			entityNew->skill[1] = entityToCopy->skill[1];
			entityNew->skill[2] = entityToCopy->skill[2];
			entityNew->skill[3] = entityToCopy->skill[3];
			entityNew->skill[4] = entityToCopy->skill[4];
			entityNew->skill[5] = entityToCopy->skill[5];
			entityNew->skill[9] = entityToCopy->skill[9];
		}
		else
		{
			// set default new entity attributes.
			entityNew->skill[0] = 0;
			entityNew->skill[1] = 1;
			entityNew->skill[2] = 1;
			entityNew->skill[3] = 1;
			entityNew->skill[4] = 0;
			entityNew->skill[5] = 0;
			entityNew->skill[9] = 0;
		}
	}
	// power crystal
	else if ( spriteType == 5 )
	{
		if ( entityToCopy != nullptr )
		{
			// copy old entity attributes to newly created.
			entityNew->yaw = entityToCopy->yaw;
			entityNew->crystalNumElectricityNodes = entityToCopy->crystalNumElectricityNodes;
			entityNew->crystalTurnReverse = entityToCopy->crystalTurnReverse;
			entityNew->crystalSpellToActivate = entityToCopy->crystalSpellToActivate;
		}
		else
		{
			// set default new entity attributes.
			entityNew->yaw = 0;
			entityNew->crystalNumElectricityNodes = 5;
			entityNew->crystalTurnReverse = 0;
			entityNew->crystalSpellToActivate = 0;
		}
	}
	// lever timer
	else if ( spriteType == 6 )
	{
		if ( entityToCopy != nullptr )
		{
			// copy old entity attributes to newly created.
			entityNew->leverTimerTicks = entityToCopy->leverTimerTicks;
		}
		else
		{
			// set default new entity attributes.
			entityNew->leverTimerTicks = 3;
		}
	}
	// boulder trap with re-fire
	else if ( spriteType == 7 )
	{
		if ( entityToCopy != nullptr )
		{
			// copy old entity attributes to newly created.
			entityNew->boulderTrapRefireDelay = entityToCopy->boulderTrapRefireDelay;
			entityNew->boulderTrapRefireAmount = entityToCopy->boulderTrapRefireAmount;
			entityNew->boulderTrapPreDelay = entityToCopy->boulderTrapPreDelay;
		}
		else
		{
			// set default new entity attributes.
			entityNew->boulderTrapRefireDelay = 3;
			entityNew->boulderTrapRefireAmount = 0;
			entityNew->boulderTrapPreDelay = 0;
		}
	}
	// pedestal
	else if ( spriteType == 8 )
	{
		if ( entityToCopy != nullptr )
		{
			// copy old entity attributes to newly created.
			entityNew->pedestalOrbType = entityToCopy->pedestalOrbType;
			entityNew->pedestalHasOrb = entityToCopy->pedestalHasOrb;
			entityNew->pedestalInvertedPower = entityToCopy->pedestalInvertedPower;
			entityNew->pedestalInGround = entityToCopy->pedestalInGround;
			entityNew->pedestalLockOrb = entityToCopy->pedestalLockOrb;
		}
		else
		{
			// set default new entity attributes.
			entityNew->pedestalOrbType = 0;
			entityNew->pedestalHasOrb = 0;
			entityNew->pedestalInvertedPower = 0;
			entityNew->pedestalInGround = 0;
			entityNew->pedestalLockOrb = 0;
		}
	}
	// teleporter
	else if ( spriteType == 9 )
	{
		if ( entityToCopy != nullptr )
		{
			// copy old entity attributes to newly created.
			entityNew->teleporterX = entityToCopy->teleporterX;
			entityNew->teleporterY = entityToCopy->teleporterY;
			entityNew->teleporterType = entityToCopy->teleporterType;
		}
		else
		{
			// set default new entity attributes.
			entityNew->teleporterX = 1;
			entityNew->teleporterY = 1;
			entityNew->teleporterType = 0;
		}
	}
	// ceiling tile
	else if ( spriteType == 10 )
	{
		if ( entityToCopy != nullptr )
		{
			// copy old entity attributes to newly created.
			entityNew->ceilingTileModel = entityToCopy->ceilingTileModel;
			entityNew->ceilingTileDir = entityToCopy->ceilingTileDir;
			entityNew->ceilingTileAllowTrap = entityToCopy->ceilingTileAllowTrap;
			entityNew->ceilingTileBreakable = entityToCopy->ceilingTileBreakable;
		}
		else
		{
			// set default new entity attributes.
			entityNew->ceilingTileModel = 0;
			entityNew->ceilingTileDir = 0;
			entityNew->ceilingTileAllowTrap = 0;
			entityNew->ceilingTileBreakable = 0;
		}
	}
	// spell trap
	else if ( spriteType == 11 )
	{
		if ( entityToCopy != nullptr )
		{
			// copy old entity attributes to newly created.
			entityNew->spellTrapType = entityToCopy->spellTrapType;
			entityNew->spellTrapRefire = entityToCopy->spellTrapRefire;
			entityNew->spellTrapLatchPower = entityToCopy->spellTrapLatchPower;
			entityNew->spellTrapFloorTile = entityToCopy->spellTrapFloorTile;
			entityNew->spellTrapRefireRate = entityToCopy->spellTrapRefireRate;
		}
		else
		{
			// set default new entity attributes.
			// copy old entity attributes to newly created.
			entityNew->spellTrapType = -1;
			entityNew->spellTrapRefire = -1;
			entityNew->spellTrapLatchPower = 0;
			entityNew->spellTrapFloorTile = 0;
			entityNew->spellTrapRefireRate = 1;
		}
	}
	// furniture
	else if ( spriteType == 12 )
	{
		if ( entityToCopy != nullptr )
		{
			// copy old entity attributes to newly created.
			entityNew->furnitureDir = entityToCopy->furnitureDir;
		}
		else
		{
			// set default new entity attributes.
			if ( entityNew->sprite == 60 ) // chair
			{
				entityNew->furnitureDir = -1;
			}
			else
			{
				entityNew->furnitureDir = 0;
			}
		}
	}
	// floor decoration
	else if ( spriteType == 13 )
	{
		if ( entityToCopy != nullptr )
		{
			// copy old entity attributes to newly created.
			entityNew->floorDecorationModel = entityToCopy->floorDecorationModel;
			entityNew->floorDecorationRotation = entityToCopy->floorDecorationRotation;
			entityNew->floorDecorationHeightOffset = entityToCopy->floorDecorationHeightOffset;
			entityNew->floorDecorationXOffset = entityToCopy->floorDecorationXOffset;
			entityNew->floorDecorationYOffset = entityToCopy->floorDecorationYOffset;
			entityNew->floorDecorationDestroyIfNoWall = entityToCopy->floorDecorationDestroyIfNoWall;
			for ( int i = 8; i < 60; ++i )
			{
				entityNew->skill[i] = entityToCopy->skill[i];
			}
		}
		else
		{
			// set default new entity attributes.
			entityNew->floorDecorationModel = 0;
			entityNew->floorDecorationRotation = 0;
			entityNew->floorDecorationHeightOffset = 0;
			entityNew->floorDecorationXOffset = 0;
			entityNew->floorDecorationYOffset = 0;
			entityNew->floorDecorationDestroyIfNoWall = -1;
			for ( int i = 8; i < 60; ++i )
			{
				entityNew->skill[i] = 0;
			}
		}
	}
	else if ( spriteType == 14 )
	{
		if ( entityToCopy != nullptr )
		{
			// copy old entity attributes to newly created.
			entityNew->soundSourceToPlay = entityToCopy->soundSourceToPlay;
			entityNew->soundSourceVolume = entityToCopy->soundSourceVolume;
			entityNew->soundSourceLatchOn = entityToCopy->soundSourceLatchOn;
			entityNew->soundSourceDelay = entityToCopy->soundSourceDelay;
			entityNew->soundSourceOrigin = entityToCopy->soundSourceOrigin;
		}
		else
		{
			// set default new entity attributes.
			entityNew->soundSourceToPlay = 0;
			entityNew->soundSourceVolume = 0;
			entityNew->soundSourceLatchOn = 0;
			entityNew->soundSourceDelay = 0;
			entityNew->soundSourceOrigin = 0;
		}
	}
	else if ( spriteType == 15 )
	{
		if ( entityToCopy != nullptr )
		{
			// copy old entity attributes to newly created.
			entityNew->lightSourceAlwaysOn = entityToCopy->lightSourceAlwaysOn;
			entityNew->lightSourceBrightness = entityToCopy->lightSourceBrightness;
			entityNew->lightSourceInvertPower = entityToCopy->lightSourceInvertPower;
			entityNew->lightSourceLatchOn = entityToCopy->lightSourceLatchOn;
			entityNew->lightSourceRadius = entityToCopy->lightSourceRadius;
			entityNew->lightSourceFlicker = entityToCopy->lightSourceFlicker;
			entityNew->lightSourceDelay = entityToCopy->lightSourceDelay;
			entityNew->lightSourceRGB = entityToCopy->lightSourceRGB;
		}
		else
		{
			// set default new entity attributes.
			entityNew->lightSourceAlwaysOn = 0;
			entityNew->lightSourceBrightness = 128;
			entityNew->lightSourceInvertPower = 0;
			entityNew->lightSourceLatchOn = 0;
			entityNew->lightSourceRadius = 5;
			entityNew->lightSourceFlicker = 0;
			entityNew->lightSourceDelay = 0;
			entityNew->lightSourceRGB = 0;
			entityNew->lightSourceRGB |= 255;
			entityNew->lightSourceRGB |= (255 << 8);
			entityNew->lightSourceRGB |= (255 << 16);
		}
	}
	else if ( spriteType == 16 )
	{
		if ( entityToCopy != nullptr )
		{
			// copy old entity attributes to newly created.
			entityNew->textSourceColorRGB = entityToCopy->textSourceColorRGB;
			entityNew->textSourceVariables4W = entityToCopy->textSourceVariables4W;
			entityNew->textSourceDelay = entityToCopy->textSourceDelay;
			entityNew->textSourceIsScript = entityToCopy->textSourceIsScript;
			for ( int i = 4; i < 60; ++i )
			{
				entityNew->skill[i] = entityToCopy->skill[i];
			}
		}
		else
		{
			// set default new entity attributes.
			entityNew->textSourceColorRGB = 0xFFFFFFFF;
			entityNew->textSourceVariables4W = 0;
			entityNew->textSourceDelay = 0;
			entityNew->textSourceIsScript = 0;
			for ( int i = 4; i < 60; ++i )
			{
				entityNew->skill[i] = 0;
			}
		}
	}
	else if ( spriteType == 17 )
	{
		if ( entityToCopy != nullptr )
		{
			// copy old entity attributes to newly created.
			entityNew->signalInputDirection = entityToCopy->signalInputDirection;
			entityNew->signalActivateDelay = entityToCopy->signalActivateDelay;
			entityNew->signalTimerInterval = entityToCopy->signalTimerInterval;
			entityNew->signalTimerRepeatCount = entityToCopy->signalTimerRepeatCount;
			entityNew->signalTimerLatchInput = entityToCopy->signalTimerLatchInput;
			entityNew->signalInvertOutput = entityToCopy->signalInvertOutput;
		}
		else
		{
			// set default new entity attributes.
			entityNew->signalInputDirection = 0;
			entityNew->signalActivateDelay = 0;
			entityNew->signalTimerInterval = 0;
			entityNew->signalTimerRepeatCount = 0;
			entityNew->signalTimerLatchInput = 0;
			entityNew->signalInvertOutput = 0;
		}
	}
	else if ( spriteType == 28 )
	{
		if ( entityToCopy != nullptr )
		{
			// copy old entity attributes to newly created.
			entityNew->signalInputDirection = entityToCopy->signalInputDirection;
			entityNew->signalActivateDelay = entityToCopy->signalActivateDelay;
			entityNew->signalTimerInterval = entityToCopy->signalTimerInterval;
			entityNew->signalTimerRepeatCount = entityToCopy->signalTimerRepeatCount;
			entityNew->signalTimerLatchInput = entityToCopy->signalTimerLatchInput;
			entityNew->signalInvertOutput = entityToCopy->signalInvertOutput;
		}
		else
		{
			// set default new entity attributes.
			entityNew->signalInputDirection = 0;
			entityNew->signalActivateDelay = 0;
			entityNew->signalTimerInterval = 0;
			entityNew->signalTimerRepeatCount = 0;
			entityNew->signalTimerLatchInput = 0;
			entityNew->signalInvertOutput = 0;
		}
	}
	else if ( spriteType == 18 )
	{
		if ( entityToCopy != nullptr )
		{
			// copy old entity attributes to newly created.
			entityNew->portalCustomSprite = entityToCopy->portalCustomSprite;
			entityNew->portalCustomSpriteAnimationFrames = entityToCopy->portalCustomSpriteAnimationFrames;
			entityNew->portalCustomZOffset = entityToCopy->portalCustomZOffset;
			entityNew->portalCustomLevelsToJump = entityToCopy->portalCustomLevelsToJump;
			entityNew->portalNotSecret = entityToCopy->portalNotSecret;
			entityNew->portalCustomRequiresPower = entityToCopy->portalCustomRequiresPower;
			for ( int i = 11; i <= 18; ++i )
			{
				entityNew->skill[i] = entityToCopy->skill[i];
			}
		}
		else
		{
			// set default new entity attributes.
			entityNew->portalCustomSprite = 161;
			entityNew->portalCustomSpriteAnimationFrames = 0;
			entityNew->portalCustomZOffset = 8;
			entityNew->portalCustomLevelsToJump = 1;
			entityNew->portalNotSecret = 1;
			entityNew->portalCustomRequiresPower = 0;
			for ( int i = 11; i <= 18; ++i )
			{
				entityNew->skill[i] = 0;
			}
		}
	}
	else if ( spriteType == 19 ) // tables
	{
		if ( entityToCopy != nullptr )
		{
			// copy old entity attributes to newly created.
			entityNew->furnitureDir = entityToCopy->furnitureDir;
			entityNew->furnitureTableSpawnChairs = entityToCopy->furnitureTableSpawnChairs;
			entityNew->furnitureTableRandomItemChance = entityToCopy->furnitureTableRandomItemChance;
		}
		else
		{
			// set default new entity attributes.
			entityNew->furnitureDir = -1;
			entityNew->furnitureTableSpawnChairs = -1;
			entityNew->furnitureTableRandomItemChance = -1;
		}
	}
	else if ( spriteType == 20 ) // readable book
	{
		if ( entityToCopy != nullptr )
		{
			// copy old entity attributes to newly created.
			entityNew->skill[11] = entityToCopy->skill[11];
			entityNew->skill[12] = entityToCopy->skill[12];
			entityNew->skill[15] = entityToCopy->skill[15];
			for ( int i = 40; i <= 52; ++i )
			{
				entityNew->skill[i] = entityToCopy->skill[i];
			}
		}
		else
		{
			// set default new entity attributes.
			entityNew->skill[11] = 0;
			entityNew->skill[12] = 10;
			entityNew->skill[15] = 0;
			for ( int i = 40; i <= 52; ++i )
			{
				entityNew->skill[i] = 0;
			}
		}
	}
	else if ( spriteType == 21 ) // doors
	{
		if ( entityToCopy != nullptr )
		{
			// copy old entity attributes to newly created.
			entityNew->doorForceLockedUnlocked = entityToCopy->doorForceLockedUnlocked;
			entityNew->doorDisableLockpicks = entityToCopy->doorDisableLockpicks;
			entityNew->doorDisableOpening= entityToCopy->doorDisableOpening;
		}
		else
		{
			// set default new entity attributes.
			entityNew->doorForceLockedUnlocked = 0;
			entityNew->doorDisableLockpicks = 0;
			entityNew->doorDisableOpening = 0;
		}
	}
	else if ( spriteType == 32 ) // iron doors
	{
		if ( entityToCopy != nullptr )
		{
			// copy old entity attributes to newly created.
			entityNew->doorUnlockWhenPowered = entityToCopy->doorUnlockWhenPowered;
			entityNew->doorDisableLockpicks = entityToCopy->doorDisableLockpicks;
			entityNew->doorForceLockedUnlocked = entityToCopy->doorForceLockedUnlocked;
			entityNew->doorDisableOpening = entityToCopy->doorDisableOpening;
		}
		else
		{
			// set default new entity attributes.
			entityNew->doorUnlockWhenPowered = 1;
			entityNew->doorDisableLockpicks = 1;
			entityNew->doorDisableOpening = 1;
			entityNew->doorForceLockedUnlocked = 1;
		}
	}
	else if ( spriteType == 22 ) // gates
	{
		if ( entityToCopy != nullptr )
		{
			// copy old entity attributes to newly created.
			entityNew->gateDisableOpening = entityToCopy->gateDisableOpening;
		}
		else
		{
			// set default new entity attributes.
			entityNew->gateDisableOpening = 0;
		}
	}
	else if ( spriteType == 23 ) // player spawns
	{
		if ( entityToCopy != nullptr )
		{
			// copy old entity attributes to newly created.
			entityNew->playerStartDir = entityToCopy->playerStartDir;
		}
		else
		{
			// set default new entity attributes.
			entityNew->playerStartDir = 0;
		}
	}
	else if ( spriteType == 24 ) // statue
	{
		if ( entityToCopy != nullptr )
		{
			// copy old entity attributes to newly created.
			entityNew->statueDir = entityToCopy->statueDir;
			entityNew->statueId = entityToCopy->statueId;
		}
		else
		{
			// set default new entity attributes.
			entityNew->statueDir = 0;
			entityNew->statueId = 0;
		}
	}
	else if ( spriteType == 25 ) // teleport shrine
	{
		if ( entityToCopy != nullptr )
		{
			// copy old entity attributes to newly created.
			entityNew->shrineDir = entityToCopy->shrineDir;
			entityNew->shrineZ = entityToCopy->shrineZ;
			entityNew->shrineDestXOffset = entityToCopy->shrineDestXOffset;
			entityNew->shrineDestYOffset = entityToCopy->shrineDestYOffset;
		}
		else
		{
			// set default new entity attributes.
			entityNew->shrineDir = 0;
			entityNew->shrineZ = 0;
			entityNew->shrineDestXOffset = 0;
			entityNew->shrineDestYOffset = 0;
		}
	}
	else if ( spriteType == 26 ) // spell shrine
	{
		if ( entityToCopy != nullptr )
		{
			// copy old entity attributes to newly created.
			entityNew->shrineDir = entityToCopy->shrineDir;
			entityNew->shrineZ = entityToCopy->shrineZ;
			entityNew->shrineDestXOffset = entityToCopy->shrineDestXOffset;
			entityNew->shrineDestYOffset = entityToCopy->shrineDestYOffset;
		}
		else
		{
			// set default new entity attributes.
			entityNew->shrineDir = 0;
			entityNew->shrineZ = 0;
			entityNew->shrineDestXOffset = 0;
			entityNew->shrineDestYOffset = 0;
		}
	}
	else if ( spriteType == 27 ) // collider deco
	{
		if ( entityToCopy != nullptr )
		{
			// copy old entity attributes to newly created.
			entityNew->colliderDecorationModel = entityToCopy->colliderDecorationModel;
			entityNew->colliderDecorationRotation = entityToCopy->colliderDecorationRotation;
			entityNew->colliderDecorationHeightOffset = entityToCopy->colliderDecorationHeightOffset;
			entityNew->colliderDecorationXOffset = entityToCopy->colliderDecorationXOffset;
			entityNew->colliderDecorationYOffset = entityToCopy->colliderDecorationYOffset;
			entityNew->colliderHasCollision = entityToCopy->colliderHasCollision;
			entityNew->colliderSizeX = entityToCopy->colliderSizeX;
			entityNew->colliderSizeY = entityToCopy->colliderSizeY;
			entityNew->colliderMaxHP = entityToCopy->colliderMaxHP;
			entityNew->colliderDiggable = entityToCopy->colliderDiggable;
			entityNew->colliderDamageTypes = entityToCopy->colliderDamageTypes;
		}
		else
		{
			// set default new entity attributes.
			entityNew->colliderDecorationModel = 0;
			entityNew->colliderDecorationRotation = 0;
			entityNew->colliderDecorationHeightOffset = 0;
			entityNew->colliderDecorationXOffset = 0;
			entityNew->colliderDecorationYOffset = 0;
			entityNew->colliderHasCollision = 1;
			entityNew->colliderSizeX = 0;
			entityNew->colliderSizeY = 0;
			entityNew->colliderMaxHP = 0;
			entityNew->colliderDiggable = 0;
			entityNew->colliderDamageTypes = 0;
		}
	}
	else if ( spriteType == 29 ) // pressure plates
	{
		if ( entityToCopy != nullptr )
		{
			// copy old entity attributes to newly created.
			entityNew->pressurePlateTriggerType = entityToCopy->pressurePlateTriggerType;
		}
		else
		{
			// set default new entity attributes.
			entityNew->pressurePlateTriggerType = 0;
		}
	}
	else if ( spriteType == 30 ) // wall locks
	{
		if ( entityToCopy != nullptr )
		{
			// copy old entity attributes to newly created.
			entityNew->wallLockMaterial = entityToCopy->wallLockMaterial;
			entityNew->wallLockInvertPower = entityToCopy->wallLockInvertPower;
			entityNew->wallLockTurnable = entityToCopy->wallLockTurnable;
			entityNew->wallLockPickable = entityToCopy->wallLockPickable;
			entityNew->wallLockPickableSkeletonKey = entityToCopy->wallLockPickableSkeletonKey;
			entityNew->wallLockAutoGenKey = entityToCopy->wallLockAutoGenKey;
		}
		else
		{
			// set default new entity attributes.
			entityNew->wallLockMaterial = 0;
			entityNew->wallLockInvertPower = 0;
			entityNew->wallLockTurnable = 0;
			entityNew->wallLockPickable = -1;
			entityNew->wallLockPickableSkeletonKey = 0;
			entityNew->wallLockAutoGenKey = 0;
		}
	}
	else if ( spriteType == 31 ) // wall buttons
	{
		if ( entityToCopy != nullptr )
		{
			// copy old entity attributes to newly created.
			entityNew->wallLockInvertPower = entityToCopy->wallLockInvertPower;
			entityNew->wallLockTimer = entityToCopy->wallLockTimer;
		}
		else
		{
			// set default new entity attributes.
			entityNew->wallLockInvertPower = 0;
			entityNew->wallLockTimer = 0;
		}
	}
	else if ( spriteType == 33 ) // wind
	{
		if ( entityToCopy != nullptr )
		{
			// copy old entity attributes to newly created.
			entityNew->skill[0] = entityToCopy->skill[0];
		}
		else
		{
			// set default new entity attributes.
			entityNew->skill[0] = 0;
		}
	}

	if ( entityToCopy != nullptr )
	{
		// if we are duplicating sprite, then copy the x and y coordinates.
		entityNew->x = entityToCopy->x;
		entityNew->y = entityToCopy->y;
	}
	else
	{
		// new entity, will follow the mouse movements when created.
	}
}
