/*-------------------------------------------------------------------------------

	BARONY
	File: entity_editor.cpp
	Desc: implements a dummy entity class so the editor can compile
	Yes, this file is an utter bodge. If a better solution can be found, great!

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/
#include "entity.hpp"



Entity::Entity(Sint32 in_sprite, Uint32 pos, list_t* entlist, list_t* creaturelist) :
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
	monsterLastDistractedByNoisemaker(skill[55]),
	monsterExtraReflexTick(skill[56]),
	monsterSentrybotLookDir(fskill[10]),
	monsterKnockbackTangentDir(fskill[11]),
	playerStrafeVelocity(fskill[12]),
	playerStrafeDir(fskill[13]),
	monsterSpecialAttackUnequipSafeguard(fskill[14]),
	entityShowOnMap(skill[59]),
	effectPolymorph(skill[50]),
	effectShapeshift(skill[53]),
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

	myCreatureListNode = nullptr;

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
	for (c = 0; c < NUMENTITYFSKILLS; ++c)
	{
		fskill[c] = 0;
	}
	skill[2] = -1;
	for (c = 0; c < 24; ++c)
	{
		flags[c] = false;
	}
	if (entlist == map.entities)
	{
		if (multiplayer != CLIENT || loading)
		{
			uid = entity_uids;
			entity_uids++;
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

	clientStats = nullptr;
	clientsHaveItsStats = false;
}

Entity::~Entity()
{
	if ( clientStats )
	{
		delete clientStats;
	}
}

Stat* Entity::getStats() const
{

	if ( this->children.first != nullptr )
	{
		if ( this->children.first->next != nullptr )
		{
			return (Stat*)this->children.first->next->element;
		}
	}


	return nullptr;
}

void Entity::addToCreatureList(list_t *list)
{
	return;
}

bool Entity::isInvisible() const
{
	//Dummy function.
	return false;
}

void actMonster(Entity* my)
{
	return;
}

void actPlayer(Entity* my)
{
	return;
}

void actSpriteNametag(Entity* my)
{
	// dummy function
	return;
}

void actFlame(Entity* my)
{
	// dummy function
	return;
}

void actSpriteWorldTooltip(Entity* my)
{
	// dummy function
	return;
}

void actDamageGib(Entity* my)
{
	// dummy function
	return;
}

void actSpriteWorldTooltip(Entity* my);

void actGoldBag(Entity* my)
{
	// dummy function
	return;
}

void actSwitch(Entity* my)
{
	// dummy function
	return;
}

void actItem(Entity* my)
{
	// dummy function
	return;
}

int playerEntityMatchesUid(Uint32 uid)
{
	return -1;
}

bool Entity::isPlayerHeadSprite() const
{
	// dummy function
	return false;
}

bool Entity::isPlayerHeadSprite(int sprite)
{
	// dummy function
	return false;
}

bool monsterChangesColorWhenAlly(Stat* myStats, Entity* entity)
{
	// dummy function
	return false;
}

Entity* uidToEntity(Sint32 uidnum)
{
	return nullptr;
}

bool Entity::bEntityHighlightedForPlayer(const int player) const
{
	return false;
}