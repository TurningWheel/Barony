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

static const int STATISTICS_BOMB_SQUAD = 0;
static const int STATISTICS_SITTING_DUCK = 1;
static const int STATISTICS_YES_WE_CAN = 2;
static const int STATISTICS_FIRE_MAYBE_DIFFERENT = 3;
static const int STATISTICS_HOT_TUB_TIME_MACHINE = 4;
static const int STATISTICS_HEAL_BOT = 5;
static const int STATISTICS_TEMPT_FATE = 6;
static const int STATISTICS_ALCHEMY_RECIPES = 7;
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
	STEAM_STAT_KILL_COMMAND
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
	std::make_pair("BARONY_ACH_KILL_COMMAND", 50)			// STEAM_STAT_KILL_COMMAND
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
extern Uint32 loadingsavegame;
extern Sint32 conductGameChallenges[NUM_CONDUCT_CHALLENGES];
extern Sint32 gameStatistics[NUM_GAMEPLAY_STATISTICS];
extern std::vector<std::pair<Uint32, Uint32>> achievementRhythmOfTheKnightVec[MAXPLAYERS];
extern bool achievementStatusRhythmOfTheKnight[MAXPLAYERS];
extern std::pair<Uint32, Uint32> achievementThankTheTankPair[MAXPLAYERS];
extern std::unordered_set<int> clientLearnedAlchemyIngredients;
extern bool achievementStatusThankTheTank[MAXPLAYERS];
extern std::vector<Uint32> achievementStrobeVec[MAXPLAYERS];
extern bool achievementStatusStrobe[MAXPLAYERS];
extern bool achievementBrawlerMode;

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
static const int SAVE_GAMES_MAX = 10;

#ifdef STEAMWORKS
bool steamLeaderboardSetScore(score_t* score);
bool steamLeaderboardReadScore(int tags[CSteamLeaderboards::k_numLeaderboardTags]);
#endif // STEAMWORKS
