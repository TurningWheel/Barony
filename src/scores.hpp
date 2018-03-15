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
#define MAXTOPSCORES 20
#define NUM_CONDUCT_CHALLENGES 32
#define NUM_GAMEPLAY_STATISTICS 64

// indexes for new game conducts
static const int CONDUCT_HARDCORE = 0; // 2 state, 1 = hardcore, 0 = not.
static const int CONDUCT_CHEATS_ENABLED = 1; // 2 state, 1 = cheats enabled, 0 = not.
static const int CONDUCT_MULTIPLAYER = 2; // 2 state, 1 = multiplayer, 0 = not.
static const int CONDUCT_CLASSIC_MODE = 3; // 2 state, 1 = classic maps, 0 = not.

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

score_t* scoreConstructor();
void scoreDeconstructor(void* data);
int saveScore();
int totalScore(score_t* score);
void loadScore(int score);
void saveAllScores(const std::string& scoresfilename);
void loadAllScores(const std::string& scoresfilename);
int saveGame();
int loadGame(int player);
list_t* loadGameFollowers();
int deleteSaveGame();
bool saveGameExists(bool singleplayer);

char* getSaveGameName(bool singleplayer);
int getSaveGameType(bool singleplayer);
int getSaveGameClientnum(bool singleplayer);
Uint32 getSaveGameMapSeed(bool singleplayer);
Uint32 getSaveGameUniqueGameKey(bool singleplayer);
int getSavegameVersion(char checkstr[64]); // returns -1 on invalid version, otherwise converts to 3 digit int

void setDefaultPlayerConducts(); // init values for foodless, penniless etc.
void updatePlayerConductsInMainLoop(); // check and update conduct flags throughout game that don't require a specific action. (tracking gold, server flags etc...)

#define SAVEGAMEFILE "savegame.dat"
#define SAVEGAMEFILE2 "savegame2.dat" // saves follower data
#define SAVEGAMEFILE_MULTIPLAYER "savegame_multiplayer.dat"
#define SAVEGAMEFILE2_MULTIPLAYER "savegame2_multiplayer.dat" // saves follower data
