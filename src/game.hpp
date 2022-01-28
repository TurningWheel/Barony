/*-------------------------------------------------------------------------------

	BARONY
	File: game.hpp
	Desc: header file for the game

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#pragma once

#include <vector>
#include <random>
#include <chrono>

#ifdef STEAMWORKS
#include <steam/steam_api.h>
#include "steam.hpp"
#endif

// REMEMBER TO CHANGE THIS WITH EVERY NEW OFFICIAL VERSION!!!
#define VERSION "v3.8.0"
#define GAME_CODE

//#define MAX_FPS_LIMIT 60 //TODO: Make this configurable.
class Entity;

#define DEBUG 1
#define ENTITY_PACKET_LENGTH 46
#define NET_PACKET_SIZE 512

// impulses (bound keystrokes, mousestrokes, and joystick/game controller strokes) //TODO: Player-by-player basis.
extern Uint32 impulses[NUMIMPULSES];
extern Uint32 joyimpulses[NUM_JOY_IMPULSES]; //Joystick/gamepad only impulses.
extern int reversemouse;
extern real_t mousespeed;

void handleEvents(void);
void startMessages();

// net packet send
typedef struct packetsend_t
{
	UDPsocket sock;
	int channel;
	UDPpacket* packet;
	int num;
	int tries;
	int hostnum;
} packetsend_t;
extern list_t safePacketsSent;
extern std::unordered_map<int, Uint32> safePacketsReceivedMap[MAXPLAYERS];
extern bool receivedclientnum;

extern Uint32 clientplayer;
extern Sint32 numplayers;
extern Sint32 clientnum;
extern bool intro;
extern int introstage;
extern bool gamePaused;
extern bool fadeout, fadefinished;
extern int fadealpha;
extern Entity* client_selected[MAXPLAYERS];
extern bool inrange[MAXPLAYERS];
extern bool deleteallbuttons;
extern Sint32 client_classes[MAXPLAYERS];
extern Uint32 client_keepalive[MAXPLAYERS];
extern Uint16 portnumber;
extern list_t messages;
extern list_t command_history;
extern node_t* chosen_command;
extern bool command;
extern bool noclip, godmode, buddhamode;
extern bool everybodyfriendly;
extern bool combat, combattoggle;
extern bool assailant[MAXPLAYERS];
extern bool oassailant[MAXPLAYERS];
extern int assailantTimer[MAXPLAYERS];
static const int COMBAT_MUSIC_COOLDOWN = 200; // 200 ticks of combat music before it fades away.
extern list_t removedEntities;
extern list_t entitiesToDelete[MAXPLAYERS];
extern char maptoload[256], configtoload[256];
extern bool loadingmap, loadingconfig;
extern int startfloor;
extern bool skipintro;
extern Uint32 uniqueGameKey;
extern bool arachnophobia_filter;

// definitions
extern bool showfps;
extern real_t time_diff;
extern real_t t, ot, frameval[AVERAGEFRAMES];
extern Uint32 cycles, pingtime;
extern real_t fps;
static const int NUMCLASSES = 21;
#define NUMRACES 13
#define NUMPLAYABLERACES 9
extern char address[64];
extern bool loadnextlevel;
extern int skipLevelsOnLoad;
extern bool loadingSameLevelAsCurrent;
extern std::string loadCustomNextMap;
extern Uint32 forceMapSeed;
extern int currentlevel;
extern bool secretlevel;
extern bool darkmap;
extern int shaking, bobbing;
extern SDL_Surface* title_bmp;
extern SDL_Surface* titleDefault_bmp;
extern SDL_Surface* logo_bmp;
extern SDL_Surface* cursor_bmp;
extern SDL_Surface* cross_bmp;
extern SDL_Surface* selected_cursor_bmp;

enum MessageType : Uint32 {
	MESSAGE_COMBAT = 1u << 0, // damage received or given in combat
	MESSAGE_STATUS = 1u << 1, // character status changes and passive effects
	MESSAGE_INVENTORY = 1u << 2, // inventory and item appraisal
	MESSAGE_EQUIPMENT = 1u << 3, // player equipment changes
	MESSAGE_WORLD = 1u << 4, // diegetic messages, such as speech and text
	MESSAGE_CHAT = 1u << 5, // multiplayer chat
	MESSAGE_PROGRESSION = 1u << 6, // player character progression messages (ie level-ups)
	MESSAGE_INTERACTION = 1u << 7, // player interactions with the world
	MESSAGE_INSPECTION = 1u << 8, // player inspections of world objects
	MESSAGE_HINT = 1u << 9, // special text cues and descriptive messages
	MESSAGE_OBITUARY = 1u << 10, // character death announcement
	MESSAGE_DEBUG = 1u << 30, // debug only messages
	MESSAGE_MISC = 1u << 31, // miscellaneous messages
};
extern Uint32 messagesEnabled;

enum PlayerClasses : int
{
	CLASS_BARBARIAN,
	CLASS_WARRIOR,
	CLASS_HEALER,
	CLASS_ROGUE,
	CLASS_WANDERER,
	CLASS_CLERIC,
	CLASS_MERCHANT,
	CLASS_WIZARD,
	CLASS_ARCANIST,
	CLASS_JOKER,
	CLASS_SEXTON,
	CLASS_NINJA,
	CLASS_MONK,
	CLASS_CONJURER,
	CLASS_ACCURSED,
	CLASS_MESMER,
	CLASS_BREWER,
	CLASS_MACHINIST,
	CLASS_PUNISHER,
	CLASS_SHAMAN,
	CLASS_HUNTER
};

static const int CLASS_SHAMAN_NUM_STARTING_SPELLS = 15;

enum PlayerRaces : int
{
	RACE_HUMAN,
	RACE_SKELETON,
	RACE_VAMPIRE,
	RACE_SUCCUBUS,
	RACE_GOATMAN,
	RACE_AUTOMATON,
	RACE_INCUBUS,
	RACE_GOBLIN,
	RACE_INSECTOID,
	RACE_RAT,
	RACE_TROLL,
	RACE_SPIDER,
	RACE_IMP
};

enum ESteamLeaderboardTitles : int
{
	LEADERBOARD_NONE,
	LEADERBOARD_NORMAL_TIME,
	LEADERBOARD_NORMAL_SCORE,
	LEADERBOARD_MULTIPLAYER_TIME,
	LEADERBOARD_MULTIPLAYER_SCORE,
	LEADERBOARD_HELL_TIME,
	LEADERBOARD_HELL_SCORE,
	LEADERBOARD_HARDCORE_TIME,
	LEADERBOARD_HARDCORE_SCORE,
	LEADERBOARD_CLASSIC_TIME,
	LEADERBOARD_CLASSIC_SCORE,
	LEADERBOARD_CLASSIC_HARDCORE_TIME,
	LEADERBOARD_CLASSIC_HARDCORE_SCORE,
	LEADERBOARD_MULTIPLAYER_CLASSIC_TIME,
	LEADERBOARD_MULTIPLAYER_CLASSIC_SCORE,
	LEADERBOARD_MULTIPLAYER_HELL_TIME,
	LEADERBOARD_MULTIPLAYER_HELL_SCORE,
	LEADERBOARD_DLC_NORMAL_TIME,
	LEADERBOARD_DLC_NORMAL_SCORE,
	LEADERBOARD_DLC_MULTIPLAYER_TIME,
	LEADERBOARD_DLC_MULTIPLAYER_SCORE,
	LEADERBOARD_DLC_HELL_TIME,
	LEADERBOARD_DLC_HELL_SCORE,
	LEADERBOARD_DLC_HARDCORE_TIME,
	LEADERBOARD_DLC_HARDCORE_SCORE,
	LEADERBOARD_DLC_CLASSIC_TIME,
	LEADERBOARD_DLC_CLASSIC_SCORE,
	LEADERBOARD_DLC_CLASSIC_HARDCORE_TIME,
	LEADERBOARD_DLC_CLASSIC_HARDCORE_SCORE,
	LEADERBOARD_DLC_MULTIPLAYER_CLASSIC_TIME,
	LEADERBOARD_DLC_MULTIPLAYER_CLASSIC_SCORE,
	LEADERBOARD_DLC_MULTIPLAYER_HELL_TIME,
	LEADERBOARD_DLC_MULTIPLAYER_HELL_SCORE
};

bool achievementUnlocked(const char* achName);
void steamAchievement(const char* achName);
void steamUnsetAchievement(const char* achName);
void steamAchievementClient(int player, const char* achName);
void steamAchievementEntity(Entity* my, const char* achName); // give steam achievement to an entity, and check for valid player info.
void steamStatisticUpdate(int statisticNum, ESteamStatTypes type, int value);
void steamStatisticUpdateClient(int player, int statisticNum, ESteamStatTypes type, int value);
void steamIndicateStatisticProgress(int statisticNum, ESteamStatTypes type);
void freePlayerEquipment(int x);
void pauseGame(int mode, int ignoreplayer);
int initGame();
void deinitGame();
Uint32 timerCallback(Uint32 interval, void* param);
void handleButtons(void);
void gameLogic(void);

// behavior function prototypes:
void actAnimator(Entity* my);
void actRotate(Entity* my);
void actLiquid(Entity* my);
void actEmpty(Entity* my);
void actFurniture(Entity* my);
void actMCaxe(Entity* my);
void actStatueAnimator(Entity* my);
void actStatue(Entity* my);
void actDoorFrame(Entity* my);
void actDeathCam(Entity* my);
void actPlayerLimb(Entity* my);
void actTorch(Entity* my);
void actCrystalShard(Entity* my);
void actDoor(Entity* my);
void actHudWeapon(Entity* my);
void actHudArm(Entity* my);
void actHudShield(Entity* my);
void actHudAdditional(Entity* my);
void actHudArrowModel(Entity* my);
void actItem(Entity* my);
void actGoldBag(Entity* my);
void actGib(Entity* my);
void actDamageGib(Entity* my);
Entity* spawnGib(Entity* parentent, int customGibSprite = -1);
Entity* spawnDamageGib(Entity* parentent, Sint32 dmgAmount);
Entity* spawnGibClient(Sint16 x, Sint16 y, Sint16 z, Sint16 sprite);
void serverSpawnGibForClient(Entity* gib);
void actLadder(Entity* my);
void actLadderUp(Entity* my);
void actPortal(Entity* my);
void actWinningPortal(Entity* my);
void actFlame(Entity* my);
void actCampfire(Entity* my);
Entity* spawnFlame(Entity* parentent, Sint32 sprite);
void actMagic(Entity* my);
Entity* castMagic(Entity* parentent);
void actSprite(Entity* my);
void actSpriteNametag(Entity* my);
void actSpriteWorldTooltip(Entity* my);
void actSleepZ(Entity* my);
Entity* spawnBang(Sint16 x, Sint16 y, Sint16 z);
Entity* spawnExplosion(Sint16 x, Sint16 y, Sint16 z);
Entity* spawnExplosionFromSprite(Uint16 sprite, Sint16 x, Sint16 y, Sint16 z);
Entity* spawnSleepZ(Sint16 x, Sint16 y, Sint16 z);
Entity* spawnFloatingSpriteMisc(int sprite, Sint16 x, Sint16 y, Sint16 z);
void actArrow(Entity* my);
void actBoulder(Entity* my);
void actBoulderTrap(Entity* my);
void actBoulderTrapEast(Entity* my);
void actBoulderTrapWest(Entity* my);
void actBoulderTrapSouth(Entity* my);
void actBoulderTrapNorth(Entity* my);
void actHeadstone(Entity* my);
void actThrown(Entity* my);
void actBeartrap(Entity* my);
void actBeartrapLaunched(Entity* my);
void actBomb(Entity* my);
void actDecoyBox(Entity* my);
void actDecoyBoxCrank(Entity* my);
void actSpearTrap(Entity* my);
void actWallBuster(Entity* my);
void actWallBuilder(Entity* my);
void actPowerCrystalBase(Entity* my);
void actPowerCrystal(Entity* my);
void actPowerCrystalParticleIdle(Entity* my);
void actPedestalBase(Entity* my);
void actPedestalOrb(Entity* my);
void actMidGamePortal(Entity* my);
void actCustomPortal(Entity* my);
void actTeleporter(Entity* my);
void actMagicTrapCeiling(Entity* my);
void actExpansionEndGamePortal(Entity* my);
void actSoundSource(Entity* my);
void actLightSource(Entity* my);
void actSignalTimer(Entity* my);

void startMessages();
bool frameRateLimit(Uint32 maxFrameRate, bool resetAccumulator = true);
extern Uint32 networkTickrate;
extern bool gameloopFreezeEntities;
extern Uint32 serverSchedulePlayerHealthUpdate;

#define TOUCHRANGE 32
#define STRIKERANGE 24
#define XPSHARERANGE 256

// function prototypes for charclass.c:
void initClass(int player);
void initShapeshiftHotbar(int player);
void deinitShapeshiftHotbar(int player);
bool playerUnlockedShamanSpell(int player, Item* item);

extern char last_ip[64];
extern char last_port[64];

//TODO: Maybe increase with level or something?
//TODO: Pause health regen during combat?
#define HEAL_TIME 600 //10 seconds. //Original time: 3600 (1 minute)
#define MAGIC_REGEN_TIME 300 // 5 seconds

#define DEFAULT_HP 30
#define DEFAULT_MP 30
#define HP_MOD 5
#define MP_MOD 5

#define SPRITE_FLAME 13
#define SPRITE_CRYSTALFLAME 96

#define MAXCHARGE 30 // charging up weapons

static const int BASE_MELEE_DAMAGE = 8;
static const int BASE_RANGED_DAMAGE = 7;
static const int BASE_THROWN_DAMAGE = 6;
static const int BASE_PLAYER_UNARMED_DAMAGE = 8;

extern bool spawn_blood;
extern bool capture_mouse; //Useful for debugging when the game refuses to release the mouse when it's crashed.

#define LEVELSFILE "maps/levels.txt"
#define SECRETLEVELSFILE "maps/secretlevels.txt"
#define LENGTH_OF_LEVEL_REGION 5

#define TICKS_PER_SECOND 50
static const Uint8 TICKS_TO_PROCESS_FIRE = 30; // The amount of ticks needed until the 'BURNING' Status Effect is processed (char_fire % TICKS_TO_PROCESS_FIRE == 0)
static const int EFFECT_WITHDRAWAL_BASE_TIME = TICKS_PER_SECOND * 60 * 8; // 8 minutes base withdrawal time.

static const std::string PLAYERNAMES_MALE_FILE = "playernames-male.txt";
static const std::string PLAYERNAMES_FEMALE_FILE = "playernames-female.txt";
extern std::vector<std::string> randomPlayerNamesMale;
extern std::vector<std::string> randomPlayerNamesFemale;
extern bool enabledDLCPack1;
extern bool enabledDLCPack2;
extern std::vector<std::string> physFSFilesInDirectory;
void loadRandomNames();
void mapLevel(int player);
void mapFoodOnLevel(int player);

class TileEntityListHandler
{
private:
	static const int kMaxMapDimension = 256;
public:
	list_t gridEntities[kMaxMapDimension][kMaxMapDimension];

	void clearTile(int x, int y);
	void emptyGridEntities();
	list_t* getTileList(int x, int y);
	node_t* addEntity(Entity& entity);
	node_t* updateEntity(Entity& entity);
	std::vector<list_t*> getEntitiesWithinRadius(int u, int v, int radius);
	std::vector<list_t*> getEntitiesWithinRadiusAroundEntity(Entity* entity, int radius);

	TileEntityListHandler()
	{
		for ( int i = 0; i < kMaxMapDimension; ++i )
		{
			for ( int j = 0; j < kMaxMapDimension; ++j )
			{
				gridEntities[i][j].first = nullptr;
				gridEntities[i][j].last = nullptr;
			}
		}
	};

	~TileEntityListHandler()
	{
		for ( int i = 0; i < kMaxMapDimension; ++i )
		{
			for ( int j = 0; j < kMaxMapDimension; ++j )
			{
				clearTile(i, j);
			}
		}
	};
};
extern TileEntityListHandler TileEntityList;

extern float framerateAccumulatedTime;

class DebugStatsClass
{
public:
	std::chrono::high_resolution_clock::time_point t1StartLoop;
	std::chrono::high_resolution_clock::time_point t2PostEvents;
	std::chrono::high_resolution_clock::time_point t21PostHandleMessages;
	std::chrono::high_resolution_clock::time_point t3SteamCallbacks;
	std::chrono::high_resolution_clock::time_point t4Music;
	std::chrono::high_resolution_clock::time_point t5MainDraw;
	std::chrono::high_resolution_clock::time_point t6Messages;
	std::chrono::high_resolution_clock::time_point t7Inputs;
	std::chrono::high_resolution_clock::time_point t8Status;
	std::chrono::high_resolution_clock::time_point t9GUI;
	std::chrono::high_resolution_clock::time_point t10FrameLimiter;
	std::chrono::high_resolution_clock::time_point t11End;

	std::chrono::high_resolution_clock::time_point eventsT1;
	std::chrono::high_resolution_clock::time_point eventsT2;
	std::chrono::high_resolution_clock::time_point eventsT3;
	std::chrono::high_resolution_clock::time_point eventsT4;
	std::chrono::high_resolution_clock::time_point eventsT5;
	std::chrono::high_resolution_clock::time_point eventsT6;

	std::chrono::high_resolution_clock::time_point messagesT1;

	std::chrono::high_resolution_clock::time_point t1Stored;
	std::chrono::high_resolution_clock::time_point t2Stored;
	std::chrono::high_resolution_clock::time_point t21Stored;
	std::chrono::high_resolution_clock::time_point t3Stored;
	std::chrono::high_resolution_clock::time_point t4Stored;
	std::chrono::high_resolution_clock::time_point t5Stored;
	std::chrono::high_resolution_clock::time_point t6Stored;
	std::chrono::high_resolution_clock::time_point t7Stored;
	std::chrono::high_resolution_clock::time_point t8Stored;
	std::chrono::high_resolution_clock::time_point t9Stored;
	std::chrono::high_resolution_clock::time_point t10Stored;
	std::chrono::high_resolution_clock::time_point t11Stored;

	std::chrono::high_resolution_clock::time_point eventsT1stored;
	std::chrono::high_resolution_clock::time_point eventsT2stored;
	std::chrono::high_resolution_clock::time_point eventsT3stored;
	std::chrono::high_resolution_clock::time_point eventsT4stored;
	std::chrono::high_resolution_clock::time_point eventsT5stored;
	std::chrono::high_resolution_clock::time_point eventsT6stored;

	std::chrono::high_resolution_clock::time_point messagesT1stored;

	std::chrono::high_resolution_clock::time_point messagesT2WhileLoop;
	bool handlePacketStartLoop = false;

	std::unordered_map<unsigned long, std::pair<std::string, int>> networkPackets;
	std::unordered_map<int, int> entityUpdatePackets;

	bool displayStats = false;
	char debugOutput[1024];
	char debugEventOutput[1024];

	DebugStatsClass()
	{};

	void inline storeOldTimePoints()
	{
		t1Stored = t1StartLoop;
		t2Stored = t2PostEvents;
		t21Stored = t21PostHandleMessages;
		t3Stored = t3SteamCallbacks;
		t4Stored = t4Music;
		t5Stored = t5MainDraw;
		t6Stored = t6Messages;
		t7Stored = t7Inputs;
		t8Stored = t8Status;
		t9Stored = t9GUI;
		t10Stored = t10FrameLimiter;
		t11Stored = t11End;
		eventsT1stored = eventsT1;
		eventsT2stored = eventsT2;
		eventsT3stored = eventsT3;
		eventsT4stored = eventsT4;
		eventsT5stored = eventsT5;
		eventsT6stored = eventsT6;

		messagesT1stored = messagesT1;
	};

	void storeStats();

	void storeEventStats();
};
extern DebugStatsClass DebugStats;

class TimerExperiments
{
public:
	static bool bUseTimerInterpolation;
	static bool bIsInit;
	static real_t lerpFactor;
	static int timeDivision;
	static bool bDebug;
	struct Clock
	{
		using duration = std::chrono::milliseconds;
		using rep = duration::rep;
		using period = duration::period;
		using time_point = std::chrono::time_point<Clock>;
		static constexpr bool is_steady = true;

		static time_point now() noexcept
		{
			return time_point{ duration{ SDL_GetTicks() } };
		}
	};

	struct State
	{
		double acceleration;
		double velocity;
		double position;
		void resetMovement();
		void resetPosition();
		void normalize(real_t min, real_t max);
	};

	struct EntityStates
	{
		State x;
		State y;
		State z;
		State yaw;
		State pitch;
		State roll;
		void resetMovement();
		void resetPosition();
	};

	friend EntityStates operator+(EntityStates lhs, EntityStates rhs)
	{
		return{ lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, 
			lhs.yaw + rhs.yaw, lhs.pitch + rhs.pitch, lhs.roll + rhs.roll };
	}
	friend EntityStates operator*(EntityStates lhs, double rhs)
	{
		return{ lhs.x * rhs, lhs.y * rhs, lhs.z * rhs,
			lhs.yaw * rhs, lhs.pitch * rhs, lhs.roll * rhs };
	}
	friend State operator+(State x, State y)
	{
		return{ x.acceleration + y.acceleration, x.velocity + y.velocity, x.position + y.position };
	}
	friend State operator*(State x, double y)
	{
		return{ x.acceleration * y, x.velocity * y, x.position * y };
	}

	static void
		integrate(State& state,
			std::chrono::time_point<Clock, std::chrono::duration<double>>,
			std::chrono::duration<double> dt);

	static std::chrono::duration<long long, std::ratio<1, 60>> dt;
	using duration = decltype(Clock::duration{} +dt);
	using time_point = std::chrono::time_point<Clock, duration>;

	static time_point timepoint;
	static time_point currentTime;
	static duration accumulator;

	static EntityStates cameraPreviousState[MAXPLAYERS];
	static EntityStates cameraCurrentState[MAXPLAYERS];
	static EntityStates cameraRenderState[MAXPLAYERS];

	static std::string render(State state);

	static void reset();
	static void updateClocks();
	static real_t lerpAngle(real_t angle1, real_t angle2, real_t alpha);
	static void renderCameras(view_t& camera, int player);
	static void postRenderRestore(view_t& camera, int player);
	static void updateEntityInterpolationPosition(Entity* entity);
};
