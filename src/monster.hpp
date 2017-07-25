/*-------------------------------------------------------------------------------

	BARONY
	File: monster.hpp
	Desc: header file for monsters

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#pragma once

enum Monster : int
{
	NOTHING,
	HUMAN,
	RAT,
	GOBLIN,
	SLIME,
	TROLL,
	OCTOPUS,
	SPIDER,
	GHOUL,
	SKELETON,
	SCORPION,
	CREATURE_IMP, //Because Apple so unkindly is already using the IMP keyword.
	BUGBEAR,
	GNOME,
	DEMON,
	SUCCUBUS,
	MIMIC,
	LICH,
	MINOTAUR,
	DEVIL,
	SHOPKEEPER,
	KOBOLD,
	SCARAB,
	CRYSTALGOLEM,
	INCUBUS,
	VAMPIRE,
	SHADOW,
	COCKATRICE,
	INSECTOID,
	GOATMAN,
	AUTOMATON,
	LICH_ICE,
	LICH_FIRE
};
const int NUMMONSTERS = 33;
extern int kills[NUMMONSTERS];

static char monstertypename[][15] =
{
	"nothing",
	"human",
	"rat",
	"goblin",
	"slime",
	"troll",
	"octopus",
	"spider",
	"ghoul",
	"skeleton",
	"scorpion",
	"imp",
	"bugbear",
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
	"lichice",
	"lichfire"

};

// body part focal points
extern float limbs[NUMMONSTERS][20][3];

// 0: nothing
// 1: red blood
// 2: green blood
// 3: slime
static char gibtype[NUMMONSTERS] =
{
	0,	//NOTHING,
	1,	//HUMAN,
	1,	//RAT,
	1,	//GOBLIN,
	3,	//SLIME,
	1,	//TROLL,
	1,	//OCTOPUS,
	2,	//SPIDER,
	2,	//GHOUL,
	0,	//SKELETON,
	2,	//SCORPION,
	1,	//CREATURE_IMP
	1,	//BUGBEAR,
	1,	//GNOME,
	1,	//DEMON,
	1,	//SUCCUBUS,
	1,	//MIMIC,
	2,	//LICH,
	1,	//MINOTAUR,
	1,	//DEVIL,
	1,	//SHOPKEEPER,
	1,	//KOBOLD,
	2,	//SCARAB,
	0,	//CRYSTALGOLem,
	1,	//INCUBUS,
	1,	//VAMPIRE,
	0,	//SHADOW,
	1,	//COCKATRICE
	2,	//INSECTOID,
	1,	//GOATMAN,
	0,	//AUTOMATON,
	2,	//LICH_ICE,
	2	//LICH_FIRE

};

// columns go like this:
// sword, mace, axe, polearm, ranged, magic
// lower number means less effective, higher number means more effective
static double damagetables[NUMMONSTERS][6] =
{
	{ 1.f, 1.f, 1.f, 1.f, 1.f, 1.f }, // nothing
	{ 1.f, 1.f, 1.f, 1.f, 1.f, 1.f }, // human
	{ 1.1, 1.1, 0.9, 0.9, 1.2, 1.f }, // rat
	{ 0.9, 1.f, 1.1, 1.1, 1.1, 1.f }, // goblin
	{ 1.4, 0.5, 1.3, 0.7, 0.5, 1.3 }, // slime
	{ 1.1, 0.8, 1.1, 0.8, 0.9, 1.f }, // troll
	{ 1.2, 1.f, 1.1, 0.9, 1.1, 1.f }, // octopus
	{ 1.f, 1.1, 1.f, 1.2, 1.1, 1.f }, // spider
	{ 1.f, 1.2, 0.8, 1.1, 0.6, 0.8 }, // ghoul
	{ 0.5, 1.4, 0.8, 1.3, 0.5, 0.8 }, // skeleton
	{ 0.9, 1.1, 1.f, 1.3, 1.f, 1.f }, // scorpion
	{ 1.1, 1.f, 0.8, 1.f, 1.f, 1.2 }, // imp
	{ 1.1, 1.f, 1.1, 0.9, 1.1, 1.f }, // bugbear
	{ 0.9, 1.f, 1.f, 0.9, 1.1, 1.1 }, // gnome
	{ 0.9, 0.8, 1.f, 0.8, 0.9, 1.1 }, // demon
	{ 1.2, 1.f, 1.f, 0.9, 1.f, 0.8 }, // succubus
	{ 0.8, 1.1, 1.3, 1.f, 0.7, 1.2 }, // mimic
	{ 2.5, 2.5, 2.5, 2.5, 1.f, 1.f }, // lich
	{ 1.f, 1.f, 1.f, 1.f, 1.f, 1.f }, // minotaur
	{ 2.f, 2.f, 2.f, 2.f, 1.f, 1.f }, // devil
	{ 0.5, 0.5, 0.5, 0.5, 0.5, 0.5 }, // shopkeeper
	{ 0.9, 1.f, 1.f, 0.9, 1.1, 1.1 }, // kobold
	{ 1.1, 1.1, 0.9, 0.9, 1.2, 1.f }, // scarab
	{ 1.1, 0.8, 1.1, 0.8, 0.9, 1.f }, // crystal golem
	{ 1.2, 1.f, 1.f, 0.9, 1.f, 0.8 }, // incubus
	{ 0.5, 1.4, 0.8, 1.3, 0.5, 0.8 }, // vampire
	{ 0.9, 1.f, 1.1, 1.1, 1.1, 1.f }, // shadow
	{ 1.1, 1.f, 0.8, 1.f, 1.f, 1.2 }, // cockatrice
	{ 0.9, 1.f, 1.1, 1.1, 1.1, 1.f }, // insectoid
	{ 0.9, 1.f, 1.1, 1.1, 1.1, 1.f }, // goatman
	{ 0.5, 1.4, 0.8, 1.3, 0.5, 0.8 }, // automaton
	{ 2.5, 2.5, 2.5, 2.5, 1.f, 1.f }, // lich ice
	{ 2.5, 2.5, 2.5, 2.5, 1.f, 1.f }  // lich fire

};

#define WAIT_FOLLOWDIST 48
#define HUNT_FOLLOWDIST 64

#define HITRATE 45

#define MONSTER_STATE my->skill[0] //TODO: Replace all occurence of this #define with the Sint32& in the Entity class.
#define MONSTER_TARGET my->skill[1] //TODO: Replace all occurence of this #define with the Sint32& in the Entity class.
#define MONSTER_INIT my->skill[3]
#define MONSTER_LOOKTIME my->skill[4]
#define MONSTER_NUMBER my->skill[5]
#define MONSTER_MOVETIME my->skill[6]
#define MONSTER_HITTIME my->skill[7]
#define MONSTER_ATTACK my->skill[8]
#define MONSTER_ATTACKTIME my->skill[9]
#define MONSTER_ARMBENDED my->skill[10]
#define MONSTER_SPOTSND my->skill[11]
#define MONSTER_SPOTVAR my->skill[12]
#define MONSTER_CLICKED my->skill[13]
#define MONSTER_SHOPXS my->skill[14]
#define MONSTER_SHOPYS my->skill[15]
#define MONSTER_SHOPXE my->skill[16]
#define MONSTER_SHOPYE my->skill[17]
#define MONSTER_STORETYPE my->skill[18]
#define MONSTER_IDLESND my->skill[19]

#define MONSTER_SPECIAL my->skill[29]
#define MONSTER_IDLEVAR myStats->monster_idlevar
#define MONSTER_SOUND myStats->monster_sound
#define MONSTER_VELX my->vel_x
#define MONSTER_VELY my->vel_y
#define MONSTER_VELZ my->vel_z
#define MONSTER_TARGETX my->fskill[2]
#define MONSTER_TARGETY my->fskill[3]
#define MONSTER_LOOKDIR my->fskill[4]
#define MONSTER_WEAPONYAW my->fskill[5]
#define MONSTER_FLIPPEDANGLE my->fskill[6]

void summonMonsterClient(Monster creature, long x, long y, Uint32 uid);
Entity* summonMonster(Monster creature, long x, long y);
bool monsterMoveAside(Entity* my, Entity* entity);

//--init* functions--
void initHuman(Entity* my, Stat* myStats);
void initRat(Entity* my, Stat* myStats);
void initGoblin(Entity* my, Stat* myStats);
void initSlime(Entity* my, Stat* myStats);
void initScorpion(Entity* my, Stat* myStats);
void initSuccubus(Entity* my, Stat* myStats);
void initTroll(Entity* my, Stat* myStats);
void initShopkeeper(Entity* my, Stat* myStats);
void initSkeleton(Entity* my, Stat* myStats);
void initMinotaur(Entity* my, Stat* myStats);
void initGhoul(Entity* my, Stat* myStats);
void initDemon(Entity* my, Stat* myStats);
void initSpider(Entity* my, Stat* myStats);
void initLich(Entity* my, Stat* myStats);
void initImp(Entity* my, Stat* myStats);
void initGnome(Entity* my, Stat* myStats);
void initDevil(Entity* my, Stat* myStats);
void initAutomaton(Entity* my, Stat* myStats);
void initCockatrice(Entity* my, Stat* myStats);
void initCrystalgolem(Entity* my, Stat* myStats);
void initScarab(Entity* my, Stat* myStats);
void initKobold(Entity* my, Stat* myStats);
void initShadow(Entity* my, Stat* myStats);
void initVampire(Entity* my, Stat* myStats);
void initIncubus(Entity* my, Stat* myStats);
void initInsectoid(Entity* my, Stat* myStats);
void initGoatman(Entity* my, Stat* myStats);

//--act*Limb functions--
void actHumanLimb(Entity* my);
void actGoblinLimb(Entity* my);
void actScorpionTail(Entity* my);
void actSuccubusLimb(Entity* my);
void actTrollLimb(Entity* my);
void actShopkeeperLimb(Entity* my);
void actSkeletonLimb(Entity* my);
void actMinotaurLimb(Entity* my);
void actGhoulLimb(Entity* my);
void actDemonLimb(Entity* my);
void actSpiderLimb(Entity* my);
void actLichLimb(Entity* my);
void actImpLimb(Entity* my);
void actGnomeLimb(Entity* my);
void actDevilLimb(Entity* my);
void actAutomatonLimb(Entity* my);
void actCockatriceLimb(Entity* my);
void actCrystalgolemLimb(Entity* my);
void actKoboldLimb(Entity* my);
void actShadowLimb(Entity* my);
void actVampireLimb(Entity* my);
void actIncubusLimb(Entity* my);
void actInsectoidLimb(Entity* my);
void actGoatmanLimb(Entity* my);
void actScarabLimb(Entity* my);

//--*Die functions--
void humanDie(Entity* my);
void ratDie(Entity* my);
void goblinDie(Entity* my);
void slimeDie(Entity* my);
void scorpionDie(Entity* my);
void succubusDie(Entity* my);
void trollDie(Entity* my);
void shopkeeperDie(Entity* my);
void skeletonDie(Entity* my);
void minotaurDie(Entity* my);
void ghoulDie(Entity* my);
void demonDie(Entity* my);
void spiderDie(Entity* my);
void lichDie(Entity* my);
void impDie(Entity* my);
void gnomeDie(Entity* my);
void devilDie(Entity* my);
void automatonDie(Entity* my);
void cockatriceDie(Entity* my);
void crystalgolemDie(Entity* my);
void scarabDie(Entity* my);
void koboldDie(Entity* my);
void shadowDie(Entity* my);
void vampireDie(Entity* my);
void incubusDie(Entity* my);
void insectoidDie(Entity* my);
void goatmanDie(Entity* my);

//--*MoveBodyparts functions--
void humanMoveBodyparts(Entity* my, Stat* myStats, double dist);
void ratAnimate(Entity* my, double dist);
void goblinMoveBodyparts(Entity* my, Stat* myStats, double dist);
void slimeAnimate(Entity* my, double dist);
void scorpionAnimate(Entity* my, double dist);
void succubusMoveBodyparts(Entity* my, Stat* myStats, double dist);
void trollMoveBodyparts(Entity* my, Stat* myStats, double dist);
void shopkeeperMoveBodyparts(Entity* my, Stat* myStats, double dist);
void skeletonMoveBodyparts(Entity* my, Stat* myStats, double dist);
void minotaurMoveBodyparts(Entity* my, Stat* myStats, double dist);
void ghoulMoveBodyparts(Entity* my, Stat* myStats, double dist);
void demonMoveBodyparts(Entity* my, Stat* myStats, double dist);
void spiderMoveBodyparts(Entity* my, Stat* myStats, double dist);
void lichAnimate(Entity* my, double dist);
void impMoveBodyparts(Entity* my, Stat* myStats, double dist);
void gnomeMoveBodyparts(Entity* my, Stat* myStats, double dist);
void devilMoveBodyparts(Entity* my, Stat* myStats, double dist);
void cockatriceMoveBodyparts(Entity* my, Stat* myStats, double dist);
void automatonMoveBodyparts(Entity* my, Stat* myStats, double dist);
void crystalgolemMoveBodyparts(Entity* my, Stat* myStats, double dist);
void scarabAnimate(Entity* my, Stat* myStats, double dist);
void koboldMoveBodyparts(Entity* my, Stat* myStats, double dist);
void shadowMoveBodyparts(Entity* my, Stat* myStats, double dist);
void vampireMoveBodyparts(Entity* my, Stat* myStats, double dist);
void incubusMoveBodyparts(Entity* my, Stat* myStats, double dist);
void insectoidMoveBodyparts(Entity* my, Stat* myStats, double dist);
void goatmanMoveBodyparts(Entity* my, Stat* myStats, double dist);

//--misc functions--
void actMinotaurTrap(Entity* my);
void actMinotaurTimer(Entity* my);
void actMinotaurCeilingBuster(Entity* my);
void actDemonCeilingBuster(Entity* my);

void actDevilTeleport(Entity* my);

void createMinotaurTimer(Entity* entity, map_t* map);

void actSummonTrap(Entity* my);
extern int monsterCurve(int level);
void handleMonsterAttack(Entity* my, Stat* mystats, Entity* target, double dist, int hasrangedweapon);

bool forceFollower(Entity& leader, Entity& follower);

//--special monster attack constants
static const int MONSTER_POSE_MELEE_WINDUP1 = 4;
static const int MONSTER_POSE_MELEE_WINDUP2 = 5;
static const int MONSTER_POSE_MELEE_WINDUP3 = 6;
static const int MONSTER_POSE_RANGED_WINDUP1 = 7;
static const int MONSTER_POSE_RANGED_WINDUP2 = 8;
static const int MONSTER_POSE_RANGED_WINDUP3 = 9;
static const int MONSTER_POSE_MAGIC_WINDUP1 = 10;
static const int MONSTER_POSE_MAGIC_WINDUP2 = 11;
static const int MONSTER_POSE_MAGIC_WINDUP3 = 12;
static const int MONSTER_POSE_GOLEM_SMASH = 13;

//--monster special cooldowns
static const int MONSTER_SPECIAL_COOLDOWN_GOLEM = 150;
static const int MONSTER_SPECIAL_COOLDOWN_KOBOLD = 250;

//--monster target search types
static const int MONSTER_TARGET_ENEMY = 0;
static const int MONSTER_TARGET_FRIEND = 1;
static const int MONSTER_TARGET_PLAYER = 2;

//--monster animation handler
static const int ANIMATE_YAW = 1;
static const int ANIMATE_PITCH = 2;
static const int ANIMATE_ROLL = 3;
static const int ANIMATE_WEAPON_YAW = 4;

static const int ANIMATE_DIR_POSITIVE = 1;
static const int ANIMATE_DIR_NEGATIVE = -1;
static const int ANIMATE_DIR_NONE = 0;

static const int ANIMATE_OVERSHOOT_TO_SETPOINT = 1;
static const int ANIMATE_OVERSHOOT_TO_ENDPOINT = 2;
static const int ANIMATE_OVERSHOOT_NONE = 0;

//--animates the selected limb to setpoint along the axis, at the given rate.
int limbAnimateToLimit(Entity* limb, int axis, double rate, double setpoint, bool shake, double shakerate);
//--animates the selected limb to setpoint, then endpoint along the axis, provided MONSTER_LIMB_OVERSHOOT is set
int limbAnimateWithOvershoot(Entity* limb, int axis, double setpointRate, double setpoint, double endpointRate, double endpoint, int dir);
int limbAngleWithinRange(real_t angle, double rate, double setpoint);
void handleMonsterSpecialAttack(Entity* my, Stat* myStats, Entity* target, double dist, int hasrangedweapon);
real_t normaliseAngle2PI(real_t angle);
void getTargetsAroundEntity(Entity* my, Entity* originalTarget, double distToFind, real_t angleToSearch, int searchType, list_t** list);
int numTargetsAroundEntity(Entity* my, double distToFind, real_t angleToSearch, int searchType);