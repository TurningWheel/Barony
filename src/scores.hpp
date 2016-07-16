/*-------------------------------------------------------------------------------

	BARONY
	File: scores.hpp
	Desc: header file for scores.cpp

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#pragma once

#define SCORESFILE "scores.dat"

// game score structure
#define MAXTOPSCORES 10
typedef struct score_t {
	Sint32 kills[NUMMONSTERS];
	Stat *stats;
	Sint32 classnum;
	Sint32 dungeonlevel;
	int victory;

	Uint32 completionTime;
	bool conductPenniless;
	bool conductFoodless;
	bool conductVegetarian;
	bool conductIlliterate;
} score_t;
extern list_t topscores;
extern int victory;

extern Uint32 completionTime;
extern bool conductPenniless;
extern bool conductFoodless;
extern bool conductVegetarian;
extern bool conductIlliterate;
extern list_t booksRead;
extern bool usedClass[10];
extern Uint32 loadingsavegame;

score_t *scoreConstructor();
void scoreDeconstructor(void *data);
int saveScore();
int totalScore(score_t *score);
void loadScore(int score);
void saveAllScores();
void loadAllScores();
int saveGame();
int loadGame(int player);
list_t *loadGameFollowers();
int deleteSaveGame();
bool saveGameExists();

char *getSaveGameName();
int getSaveGameType();
int getSaveGameClientnum();
Uint32 getSaveGameMapSeed();
Uint32 getSaveGameUniqueGameKey();

#define SAVEGAMEFILE "savegame.dat"
#define SAVEGAMEFILE2 "savegame2.dat" // saves follower data
