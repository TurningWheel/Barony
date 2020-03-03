/*-------------------------------------------------------------------------------

	BARONY
	File: scores.hpp
	Desc: header file for scores.cpp

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#pragma once

#define SCORESFILE "scores.dat"
#define SCORESFILE_MULTIPLAYER "scores_multiplayer.dat"

// game score structure
#define MAXTOPSCORES 30
#define NUM_CONDUCT_CHALLENGES 32
#define NUM_GAMEPLAY_STATISTICS 64

// indexes for new game conducts
static const int CONDUCT_HARDCORE = 0; // 2 state, 1 = hardcore, 0 = not.
static const int CONDUCT_CHEATS_ENABLED = 1; // 2 state, 1 = cheats enabled, 0 = not.
static const int CONDUCT_MULTIPLAYER = 2; // 2 state, 1 = multiplayer, 0 = not.
static const int CONDUCT_CLASSIC_MODE = 3; // 2 state, 1 = classic maps, 0 = not.
static const int CONDUCT_MODDED = 4; // 2 state, 1 = modded, 0 = not.
static const int CONDUCT_BRAWLER = 5; // 2 state, 1 = brawler, no weapons used, 0 = not brawler, weapons used.
static const int CONDUCT_BLESSED_BOOTS_SPEED = 6; // 2 state, 1 = reached Sanctum in < 45 mins, 0 = not.
static const int CONDUCT_BOOTS_SPEED = 7; // 2 state, 1 = beat Herx/Baphy in < 20 mins, 0 = not.
static const int CONDUCT_KEEPINVENTORY = 8; // 1 = keep inventory server flag, 0 = not.
static const int CONDUCT_LIFESAVING = 9; // 1 = lifesaving server flag, 0 = not.
static const int CONDUCT_ACCURSED = 10; // 1 = cursed, 0 = not
static const int CONDUCT_RANGED_ONLY = 11; // 1 = ranged only, 0 = not.

static const int STATISTICS_BOMB_SQUAD = 0;
static const int STATISTICS_SITTING_DUCK = 1;
static const int STATISTICS_YES_WE_CAN = 2;
static const int STATISTICS_FIRE_MAYBE_DIFFERENT = 3;
static const int STATISTICS_HOT_TUB_TIME_MACHINE = 4;
static const int STATISTICS_HEAL_BOT = 5;
static const int STATISTICS_TEMPT_FATE = 6;
static const int STATISTICS_ALCHEMY_RECIPES = 7;
static const int STATISTICS_FUNCTIONAL = 8;
static const int STATISTICS_OHAI_MARK = 9;
static const int STATISTICS_ITS_A_LIVING = 10;
static const int STATISTICS_FORUM_TROLL = 11;
static const int STATISTICS_PIMPING_AINT_EASY = 12;
static const int STATISTICS_TRIBE_SUBSCRIBE = 13;
static const int STATISTICS_POP_QUIZ_1 = 14;
static const int STATISTICS_POP_QUIZ_2 = 15;
static const int STATISTICS_DISABLE_UPLOAD = 31;

enum SteamStatIndexes : int
{
	STEAM_STAT_BOULDER_DEATHS,
	STEAM_STAT_RHINESTONE_COWBOY,
	STEAM_STAT_TOUGH_AS_NAILS,
	STEAM_STAT_UNSTOPPABLE_FORCE,
	STEAM_STAT_GAMES_STARTED,
	STEAM_STAT_GAMES_WON,
	STEAM_STAT_BOMBARDIER,
	STEAM_STAT_IN_THE_MIX,
	STEAM_STAT_FREE_REFILLS,
	STEAM_STAT_TAKE_THIS_OUTSIDE,
	STEAM_STAT_ALTER_EGO,
	STEAM_STAT_BLOOD_SPORT,
	STEAM_STAT_BAD_BLOOD,
	STEAM_STAT_IRON_GUT,
	STEAM_STAT_BOTTLE_NOSED,
	STEAM_STAT_BARFIGHT_CHAMP,
	STEAM_STAT_VOLATILE,
	STEAM_STAT_SURROGATES,
	STEAM_STAT_KILL_COMMAND,
	STEAM_STAT_TRASH_COMPACTOR,
	STEAM_STAT_SPICY,
	STEAM_STAT_SERIAL_THRILLA,
	STEAM_STAT_TRADITION,
	STEAM_STAT_POP_QUIZ,
	STEAM_STAT_DYSLEXIA,
	STEAM_STAT_BOOKWORM,
	STEAM_STAT_MONARCH,
	STEAM_STAT_SUPER_SHREDDER,
	STEAM_STAT_FIXER_UPPER,
	STEAM_STAT_TORCHERER,
	STEAM_STAT_MANY_PEDI_PALP,
	STEAM_STAT_5000_SECOND_RULE,
	STEAM_STAT_SOCIAL_BUTTERFLY,
	STEAM_STAT_ROLL_THE_BONES,
	STEAM_STAT_COWBOY_FROM_HELL,
	STEAM_STAT_SELF_FLAGELLATION,
	STEAM_STAT_CHOPPING_BLOCK,
	STEAM_STAT_IF_YOU_LOVE_SOMETHING,
	STEAM_STAT_RAGE_AGAINST,
	STEAM_STAT_GUERILLA_RADIO,
	STEAM_STAT_FASCIST,
	STEAM_STAT_ITS_A_LIVING,
	STEAM_STAT_OVERCLOCKED
};

#ifdef STEAMWORKS
static const std::pair<std::string, int> steamStatAchStringsAndMaxVals[] = 
{
	std::make_pair("BARONY_ACH_NONE", 0),					// STEAM_STAT_BOULDER_DEATHS,
	std::make_pair("BARONY_ACH_RHINESTONE_COWBOY", 50),		// STEAM_STAT_RHINESTONE_COWBOY,
	std::make_pair("BARONY_ACH_TOUGH_AS_NAILS", 50),		// STEAM_STAT_TOUGH_AS_NAILS,
	std::make_pair("BARONY_ACH_UNSTOPPABLE_FORCE",50),		// STEAM_STAT_UNSTOPPABLE_FORCE,
	std::make_pair("BARONY_ACH_NONE", 0),					// STEAM_STAT_GAMES_STARTED,
	std::make_pair("BARONY_ACH_NONE", 0),					// STEAM_STAT_GAMES_WON,
	std::make_pair("BARONY_ACH_BOMBARDIER", 50),			// STEAM_STAT_BOMBARDIER,
	std::make_pair("BARONY_ACH_IN_THE_MIX", 50),			// STEAM_STAT_IN_THE_MIX,
	std::make_pair("BARONY_ACH_FREE_REFILLS", 50),			// STEAM_STAT_FREE_REFILLS,
	std::make_pair("BARONY_ACH_TAKE_THIS_OUTSIDE", 10),		// STEAM_STAT_TAKE_THIS_OUTSIDE,
	std::make_pair("BARONY_ACH_RICH_ALTER_EGO", 50000),		// STEAM_STAT_ALTER_EGO,
	std::make_pair("BARONY_ACH_BLOOD_SPORT", 50),			// STEAM_STAT_BLOOD_SPORT,
	std::make_pair("BARONY_ACH_BAD_BLOOD", 500),			// STEAM_STAT_BAD_BLOOD,
	std::make_pair("BARONY_ACH_IRON_GUT", 20),				// STEAM_STAT_IRON_GUT,
	std::make_pair("BARONY_ACH_BOTTLE_NOSED", 20),			// STEAM_STAT_BOTTLE_NOSED,
	std::make_pair("BARONY_ACH_BARFIGHT_CHAMP", 50),		// STEAM_STAT_BARFIGHT_CHAMP,
	std::make_pair("BARONY_ACH_VOLATILE", 20),				// STEAM_STAT_VOLATILE,
	std::make_pair("BARONY_ACH_SURROGATES", 50),			// STEAM_STAT_SURROGATES,
	std::make_pair("BARONY_ACH_KILL_COMMAND", 50),			// STEAM_STAT_KILL_COMMAND
	std::make_pair("BARONY_ACH_TRASH_COMPACTOR", 100),      // STEAM_STAT_TRASH_COMPACTOR
	std::make_pair("BARONY_ACH_SPICY", 10),                 // STEAM_STAT_SPICY
	std::make_pair("BARONY_ACH_SERIAL_THRILLA", 100),       // STEAM_STAT_SERIAL_THRILLA
	std::make_pair("BARONY_ACH_TRADITION", 20),             // STEAM_STAT_TRADITION
	std::make_pair("BARONY_ACH_POP_QUIZ", 20),              // STEAM_STAT_POP_QUIZ
	std::make_pair("BARONY_ACH_DYSLEXIA", 50),              // STEAM_STAT_DYSLEXIA
	std::make_pair("BARONY_ACH_BOOKWORM", 50),              // STEAM_STAT_BOOKWORM
	std::make_pair("BARONY_ACH_MONARCH", 20),               // STEAM_STAT_MONARCH
	std::make_pair("BARONY_ACH_SUPER_SHREDDER", 1000),      // STEAM_STAT_SUPER_SHREDDER
	std::make_pair("BARONY_ACH_FIXER_UPPER", 100),          // STEAM_STAT_FIXER_UPPER
	std::make_pair("BARONY_ACH_TORCHERER", 100),            // STEAM_STAT_TORCHERER
	std::make_pair("BARONY_ACH_MANY_PEDI_PALP", 50),       // STEAM_STAT_MANY_PEDI_PALP
	std::make_pair("BARONY_ACH_5000_SECOND_RULE", 50),      // STEAM_STAT_5000_SECOND_RULE
	std::make_pair("BARONY_ACH_SOCIAL_BUTTERFLY", 50),      // STEAM_STAT_SOCIAL_BUTTERFLY
	std::make_pair("BARONY_ACH_ROLL_THE_BONES", 50),        // STEAM_STAT_ROLL_THE_BONES
	std::make_pair("BARONY_ACH_COWBOY_FROM_HELL", 50),      // STEAM_STAT_COWBOY_FROM_HELL
	std::make_pair("BARONY_ACH_SELF_FLAGELLATION", 30),     // STEAM_STAT_SELF_FLAGELLATION
	std::make_pair("BARONY_ACH_CHOPPING_BLOCK", 50),       // STEAM_STAT_CHOPPING_BLOCK
	std::make_pair("BARONY_ACH_IF_YOU_LOVE_SOMETHING", 100),// STEAM_STAT_IF_YOU_LOVE_SOMETHING
	std::make_pair("BARONY_ACH_RAGE_AGAINST", 20),          // STEAM_STAT_RAGE_AGAINST
	std::make_pair("BARONY_ACH_GUERILLA_RADIO", 20),        // STEAM_STAT_GUERILLA_RADIO
	std::make_pair("BARONY_ACH_FASCIST", 50),				// STEAM_STAT_FASCIST,
	std::make_pair("BARONY_ACH_ITS_A_LIVING", 50),			// STEAM_STAT_ITS_A_LIVING,
	std::make_pair("BARONY_ACH_OVERCLOCKED", 600)			// STEAM_STAT_OVERCLOCKED
};
#endif // STEAMWORKS

typedef struct score_t
{
	Sint32 kills[NUMMONSTERS];
	Stat* stats;
	Sint32 classnum;
	Sint32 dungeonlevel;
	int victory;

	Uint32 completionTime;
	bool conductPenniless;
	bool conductFoodless;
	bool conductVegetarian;
	bool conductIlliterate;
	Sint32 conductGameChallenges[NUM_CONDUCT_CHALLENGES];
	Sint32 gameStatistics[NUM_GAMEPLAY_STATISTICS];
} score_t;
extern list_t topscores;
extern list_t topscoresMultiplayer;
extern int victory;

extern Uint32 completionTime;
extern bool conductPenniless;
extern bool conductFoodless;
extern bool conductVegetarian;
extern bool conductIlliterate;
extern list_t booksRead;
extern bool usedClass[NUMCLASSES];
extern bool usedRace[NUMRACES];
extern Uint32 loadingsavegame;
extern Sint32 conductGameChallenges[NUM_CONDUCT_CHALLENGES];
extern Sint32 gameStatistics[NUM_GAMEPLAY_STATISTICS];
extern std::vector<std::pair<Uint32, Uint32>> achievementRhythmOfTheKnightVec[MAXPLAYERS];
extern bool achievementStatusRhythmOfTheKnight[MAXPLAYERS];
extern std::pair<Uint32, Uint32> achievementThankTheTankPair[MAXPLAYERS];
extern bool achievementStatusBaitAndSwitch[MAXPLAYERS];
extern Uint32 achievementBaitAndSwitchTimer[MAXPLAYERS];
extern std::unordered_set<int> clientLearnedAlchemyIngredients;
extern bool achievementStatusThankTheTank[MAXPLAYERS];
extern std::vector<Uint32> achievementStrobeVec[MAXPLAYERS];
extern bool achievementStatusStrobe[MAXPLAYERS];
extern bool playerFailedRangedOnlyConduct[MAXPLAYERS];
extern bool achievementBrawlerMode;
extern bool achievementRangedMode[MAXPLAYERS];

score_t* scoreConstructor();
void scoreDeconstructor(void* data);
int saveScore();
int totalScore(score_t* score);
void loadScore(int score);
void saveAllScores(const std::string& scoresfilename);
void loadAllScores(const std::string& scoresfilename);
extern int savegameCurrentFileIndex;
std::string setSaveGameFileName(bool singleplayer, bool followersFile, int saveIndex = savegameCurrentFileIndex);
int saveGame(int saveIndex = savegameCurrentFileIndex);
int loadGame(int player, int saveIndex = savegameCurrentFileIndex);
list_t* loadGameFollowers(int saveIndex = savegameCurrentFileIndex);
int deleteSaveGame(int gametype, int saveIndex = savegameCurrentFileIndex);
bool saveGameExists(bool singleplayer, int saveIndex = savegameCurrentFileIndex);
bool anySaveFileExists();

char* getSaveGameName(bool singleplayer, int saveIndex = savegameCurrentFileIndex);
int getSaveGameType(bool singleplayer, int saveIndex = savegameCurrentFileIndex);
int getSaveGameClientnum(bool singleplayer, int saveIndex = savegameCurrentFileIndex);
Uint32 getSaveGameMapSeed(bool singleplayer, int saveIndex = savegameCurrentFileIndex);
Uint32 getSaveGameUniqueGameKey(bool singleplayer, int saveIndex = savegameCurrentFileIndex);
int getSavegameVersion(char checkstr[64]); // returns -1 on invalid version, otherwise converts to 3 digit int
void setDefaultPlayerConducts(); // init values for foodless, penniless etc.
void updatePlayerConductsInMainLoop(); // check and update conduct flags throughout game that don't require a specific action. (tracking gold, server flags etc...)
void updateGameplayStatisticsInMainLoop(); // check for achievement values for gameplay statistics.
void updateAchievementRhythmOfTheKnight(int player, Entity* target, bool playerIsHit);
void updateAchievementThankTheTank(int player, Entity* target, bool targetKilled);
void updateAchievementBaitAndSwitch(int player, bool isTeleporting);
static const int SAVE_GAMES_MAX = 10;

class AchievementObserver
{
	int levelObserved = -1;
public:
	Uint32 playerUids[MAXPLAYERS] = { 0 };
	void getCurrentPlayerUids();
	bool updateOnLevelChange();
	void updateData();
	int checkUidIsFromPlayer(Uint32 uid);
	std::unordered_map<Uint32, std::unordered_map<int, std::pair<int,int>>> entityAchievementsToProcess; // uid of entity, achievement int, <ticks remaining, optional counter>
	
	bool addEntityAchievementTimer(Entity* entity, int achievement, int ticks, bool resetTimerIfActive, int optionalIncrement);

	void achievementTimersTickDown();
	void awardAchievementIfActive(int player, Entity* entity, int achievement);
	void awardAchievement(int player, int achievement);
	void printActiveAchievementTimers();

	enum Achievement : int
	{
		BARONY_ACH_TELEFRAG = 1,
		BARONY_ACH_REAL_BOY,
		BARONY_ACH_COOP_ESCAPE_MINES,
		BARONY_ACH_SWINGERS,
		BARONY_ACH_COLD_BLOODED,
		BARONY_ACH_SOULLESS,
		BARONY_ACH_TRIBAL,
		BARONY_ACH_MANAGEMENT_TEAM,
		BARONY_ACH_SOCIOPATHS,
		BARONY_ACH_FACES_OF_DEATH,
		BARONY_ACH_SURVIVALISTS,
		BARONY_ACH_BOMBTRACK,
		BARONY_ACH_CALM_LIKE_A_BOMB,
		BARONY_ACH_CAUGHT_IN_A_MOSH,
		BARONY_ACH_PLEASE_HOLD,
		BARONY_ACH_FELL_BEAST,
		BARONY_ACH_STRUNG_OUT,
		BARONY_ACH_OHAI_MARK,
		BARONY_ACH_IRONIC_PUNISHMENT,
		BARONY_ACH_LEVITANT_LACKEY,
		BARONY_ACH_WONDERFUL_TOYS,
		BARONY_ACH_FLUTTERSHY,
		BARONY_ACH_IF_YOU_LOVE_SOMETHING,
		BARONY_ACH_COWBOY_FROM_HELL,
		BARONY_ACH_TRASH_COMPACTOR
	};
	enum AchievementEvent : int
	{
		ACH_EVENT_NONE,
		REAL_BOY_HUMAN_RECRUIT,
		REAL_BOY_SHOP,
		FORUM_TROLL_BREAK_WALL,
		FORUM_TROLL_RECRUIT_TROLL,
		FORUM_TROLL_FEAR
	};
	void updatePlayerAchievement(int player, Achievement achievement, AchievementEvent achEvent);

	class PlayerAchievements
	{
	public:
		bool caughtInAMosh = false;
		bool bombTrack = false;
		bool calmLikeABomb = false;
		bool strungOut = false;
		bool wonderfulToys = false;
		bool levitantLackey = false;
		bool flutterShy = false;
		bool gastricBypass = false;
		Uint32 ticksSpentOverclocked = 0;
		bool tradition = false;
		int traditionKills = 0;
		int torchererScrap = 0;
		int superShredder = 0;
		int fixerUpper = 0;
		int ifYouLoveSomething = 0;
		int socialButterfly = 0;
		int rollTheBones = 0;
		int trashCompactor = 0;

		std::pair<int, int> realBoy;
		std::unordered_map<Uint32, int> caughtInAMoshTargets;
		std::vector<Uint32> strungOutTicks;
		std::unordered_set<Uint32> ironicPunishmentTargets;
		std::pair<real_t, real_t> flutterShyCoordinates;
		std::pair<int, Uint32> gastricBypassSpell;
		std::unordered_set<Uint32> rat5000secondRule;
		
		PlayerAchievements()
		{
			realBoy = std::make_pair(0, 0);
			gastricBypassSpell = std::make_pair(0, 0);
			flutterShyCoordinates = std::make_pair(0.0, 0.0);
		};
		bool checkPathBetweenObjects(Entity* player, Entity* target, int achievement);
		bool checkTraditionKill(Entity* player, Entity* target);
	} playerAchievements[MAXPLAYERS];

	void clearPlayerAchievementData();
};
extern AchievementObserver achievementObserver;

#ifdef STEAMWORKS
bool steamLeaderboardSetScore(score_t* score);
bool steamLeaderboardReadScore(int tags[CSteamLeaderboards::k_numLeaderboardTags]);
#endif // STEAMWORKS
