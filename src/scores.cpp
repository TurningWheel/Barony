/*-------------------------------------------------------------------------------

	BARONY
	File: scores.cpp
	Desc: contains code for handling scores, statistics, and save games

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "files.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "menu.hpp"
#include "monster.hpp"
#include "scores.hpp"
#include "items.hpp"
#include "interface/interface.hpp"
#include "magic/magic.hpp"
#include "net.hpp"
#include "player.hpp"
#include "sys/stat.h"
#include "paths.hpp"
#include "collision.hpp"
#include "mod_tools.hpp"
#include "lobbies.hpp"

// definitions
list_t topscores;
list_t topscoresMultiplayer;
int victory = false;
int actualScoreVictory = 0;
Uint32 completionTime = 0;
bool conductPenniless = true;
bool conductFoodless = true;
bool conductVegetarian = true;
bool conductIlliterate = true;
Sint32 conductGameChallenges[NUM_CONDUCT_CHALLENGES] = { 0 }; // additional 'conducts' to be stored in here.
Sint32 gameStatistics[NUM_GAMEPLAY_STATISTICS] = { 0 }; // general saved game statistics to be stored in here.
std::vector<std::pair<Uint32, Uint32>> achievementRhythmOfTheKnightVec[MAXPLAYERS] = {};
bool achievementStatusRhythmOfTheKnight[MAXPLAYERS] = { false };
std::pair<Uint32, Uint32> achievementThankTheTankPair[MAXPLAYERS] = { std::make_pair(0, 0) };
bool achievementStatusBaitAndSwitch[MAXPLAYERS] = { false };
Uint32 achievementBaitAndSwitchTimer[MAXPLAYERS] = { 0 };
std::unordered_set<int> clientLearnedAlchemyIngredients;
bool achievementStatusThankTheTank[MAXPLAYERS] = { false };
std::vector<Uint32> achievementStrobeVec[MAXPLAYERS] = {};
bool achievementStatusStrobe[MAXPLAYERS] = { false };
bool playerFailedRangedOnlyConduct[MAXPLAYERS] = { false };
list_t booksRead;
bool usedClass[NUMCLASSES] = {0};
bool usedRace[NUMRACES] = { 0 };
Uint32 loadingsavegame = 0;
bool achievementBrawlerMode = false;
bool achievementRangedMode[MAXPLAYERS] = { 0 };
int savegameCurrentFileIndex = 0;
score_t steamLeaderboardScore;
AchievementObserver achievementObserver;

/*-------------------------------------------------------------------------------

	scoreConstructor

	creates a score_t structure

-------------------------------------------------------------------------------*/

score_t* scoreConstructor()
{
	node_t* node;

	score_t* score = (score_t*) malloc(sizeof(score_t));
	if ( !score )
	{
		printlog( "failed to allocate memory for new score!\n" );
		exit(1);
	}
	// Stat set to 0 as monster type not needed, values will be overwritten by the player data
	score->stats = new Stat(0);
	if ( !score->stats )
	{
		printlog( "failed to allocate memory for new stat!\n" );
		exit(1);
	}

	// set all data elements
	int c;
	for ( c = 0; c < NUMMONSTERS; c++ )
	{
		score->kills[c] = kills[c];
	}
	score->stats->type = stats[clientnum]->type;
	score->stats->sex = stats[clientnum]->sex;
	score->stats->appearance = stats[clientnum]->appearance;
	score->stats->playerRace = stats[clientnum]->playerRace;
	//score->stats->appearance |= stats[clientnum]->playerRace << 8;
	strcpy(score->stats->name, stats[clientnum]->name);
	strcpy(score->stats->obituary, stats[clientnum]->obituary);
	score->victory = victory;
	score->dungeonlevel = currentlevel;
	score->classnum = client_classes[clientnum];
	score->stats->HP = stats[clientnum]->HP;
	score->stats->MAXHP = stats[clientnum]->MAXHP;
	score->stats->MP = stats[clientnum]->MP;
	score->stats->MAXMP = stats[clientnum]->MAXMP;
	score->stats->STR = stats[clientnum]->STR;
	score->stats->DEX = stats[clientnum]->DEX;
	score->stats->CON = stats[clientnum]->CON;
	score->stats->INT = stats[clientnum]->INT;
	score->stats->PER = stats[clientnum]->PER;
	score->stats->CHR = stats[clientnum]->CHR;
	score->stats->EXP = stats[clientnum]->EXP;
	score->stats->LVL = stats[clientnum]->LVL;
	score->stats->GOLD = stats[clientnum]->GOLD;
	score->stats->HUNGER = stats[clientnum]->HUNGER;
	for ( c = 0; c < NUMPROFICIENCIES; c++ )
	{
		score->stats->PROFICIENCIES[c] = stats[clientnum]->PROFICIENCIES[c];
	}
	for ( c = 0; c < NUMEFFECTS; c++ )
	{
		score->stats->EFFECTS[c] = stats[clientnum]->EFFECTS[c];
		score->stats->EFFECTS_TIMERS[c] = stats[clientnum]->EFFECTS_TIMERS[c];
	}
	score->stats->leader_uid = 0;
	score->stats->FOLLOWERS.first = NULL;
	score->stats->FOLLOWERS.last = NULL;
	score->stats->stache_x1 = 0;
	score->stats->stache_x2 = 0;
	score->stats->stache_y1 = 0;
	score->stats->stache_y2 = 0;
	score->stats->inventory.first = NULL;
	score->stats->inventory.last = NULL;
	score->stats->helmet = NULL;
	score->stats->breastplate = NULL;
	score->stats->gloves = NULL;
	score->stats->shoes = NULL;
	score->stats->shield = NULL;
	score->stats->weapon = NULL;
	score->stats->cloak = NULL;
	score->stats->amulet = NULL;
	score->stats->ring = NULL;
	score->stats->mask = NULL;
	list_Copy(&score->stats->inventory, &stats[clientnum]->inventory);
	for ( node = score->stats->inventory.first; node != NULL; node = node->next )
	{
		Item* item = (Item*)node->element;
		item->node = node;
	}
	for ( c = 0, node = stats[clientnum]->inventory.first; node != NULL; node = node->next, c++ )
	{
		Item* item = (Item*)node->element;
		if ( stats[clientnum]->helmet == item )
		{
			node_t* node2 = list_Node(&score->stats->inventory, c);
			Item* item2 = (Item*)node2->element;
			score->stats->helmet = item2;
		}
		else if ( stats[clientnum]->breastplate == item )
		{
			node_t* node2 = list_Node(&score->stats->inventory, c);
			Item* item2 = (Item*)node2->element;
			score->stats->breastplate = item2;
		}
		else if ( stats[clientnum]->gloves == item )
		{
			node_t* node2 = list_Node(&score->stats->inventory, c);
			Item* item2 = (Item*)node2->element;
			score->stats->gloves = item2;
		}
		else if ( stats[clientnum]->shoes == item )
		{
			node_t* node2 = list_Node(&score->stats->inventory, c);
			Item* item2 = (Item*)node2->element;
			score->stats->shoes = item2;
		}
		else if ( stats[clientnum]->shield == item )
		{
			node_t* node2 = list_Node(&score->stats->inventory, c);
			Item* item2 = (Item*)node2->element;
			score->stats->shield = item2;
		}
		else if ( stats[clientnum]->weapon == item )
		{
			node_t* node2 = list_Node(&score->stats->inventory, c);
			Item* item2 = (Item*)node2->element;
			score->stats->weapon = item2;
		}
		else if ( stats[clientnum]->cloak == item )
		{
			node_t* node2 = list_Node(&score->stats->inventory, c);
			Item* item2 = (Item*)node2->element;
			score->stats->cloak = item2;
		}
		else if ( stats[clientnum]->amulet == item )
		{
			node_t* node2 = list_Node(&score->stats->inventory, c);
			Item* item2 = (Item*)node2->element;
			score->stats->amulet = item2;
		}
		else if ( stats[clientnum]->ring == item )
		{
			node_t* node2 = list_Node(&score->stats->inventory, c);
			Item* item2 = (Item*)node2->element;
			score->stats->ring = item2;
		}
		else if ( stats[clientnum]->mask == item )
		{
			node_t* node2 = list_Node(&score->stats->inventory, c);
			Item* item2 = (Item*)node2->element;
			score->stats->mask = item2;
		}
	}
	score->stats->monster_sound = NULL;
	score->stats->monster_idlevar = 0;

	score->completionTime = completionTime;
	score->conductPenniless = conductPenniless;
	score->conductFoodless = conductFoodless;
	score->conductVegetarian = conductVegetarian;
	score->conductIlliterate = conductIlliterate;
	for ( c = 0; c < NUM_CONDUCT_CHALLENGES; ++c )
	{
		score->conductGameChallenges[c] = conductGameChallenges[c];
	}
	for ( c = 0; c < NUM_GAMEPLAY_STATISTICS; ++c )
	{
		score->gameStatistics[c] = gameStatistics[c];
	}
	return score;
}

/*-------------------------------------------------------------------------------

	scoreDeconstructor

	destroys a score_t structure

-------------------------------------------------------------------------------*/

void scoreDeconstructor(void* data)
{
	if ( data )
	{
		score_t* score = (score_t*)data;
		if ( score->stats )
		{
			delete score->stats;
		}
		//score->stats->~Stat();
		free(data);
	}
}

/*-------------------------------------------------------------------------------

	saveScore

	saves the current game score to the highscore list. Returns -1 if the
	score is not saved

-------------------------------------------------------------------------------*/

int saveScore()
{
	node_t* node;
	int c;

	score_t* currentscore = scoreConstructor();
	list_t* scoresPtr = &topscores;
	if ( conductGameChallenges[CONDUCT_MULTIPLAYER] )
	{
		scoresPtr = &topscoresMultiplayer;
	}

#ifdef STEAMWORKS
	if ( g_SteamLeaderboards )
	{
		if ( steamLeaderboardSetScore(currentscore) )
		{
			g_SteamLeaderboards->LeaderboardUpload.score = totalScore(currentscore);
			g_SteamLeaderboards->LeaderboardUpload.time = currentscore->completionTime / TICKS_PER_SECOND;
			g_SteamLeaderboards->LeaderboardUpload.status = LEADERBOARD_STATE_FIND_LEADERBOARD_TIME;
			printlog("[STEAM]: Initialising leaderboard score upload...");
		}
		else
		{
			printlog("[STEAM]: Did not qualify for leaderboard score upload.");
			if ( currentscore->gameStatistics[STATISTICS_DISABLE_UPLOAD] == 1 )
			{
				printlog("[STEAM]: Loaded data did not match hash as expected.");
			}
		}
	}
#endif // STEAMWORKS

	for ( c = 0, node = scoresPtr->first; node != NULL; node = node->next, c++ )
	{
		score_t* score = (score_t*)node->element;
		if ( totalScore(score) <= totalScore(currentscore) )
		{
			node_t* newNode = list_AddNode(scoresPtr, c);
			newNode->element = currentscore;
			newNode->deconstructor = &scoreDeconstructor;
			newNode->size = sizeof(score_t);
			while ( list_Size(scoresPtr) > MAXTOPSCORES )
			{
				list_RemoveNode(scoresPtr->last);
			}
			return c;
		}
	}
	if ( c == MAXTOPSCORES )
	{
		scoreDeconstructor((void*)currentscore);
		return -1; // do not save the score
	}
	node = list_AddNodeLast(scoresPtr);
	node->element = currentscore;
	node->deconstructor = &scoreDeconstructor;
	node->size = sizeof(score_t);

	return c;
}

/*-------------------------------------------------------------------------------

	totalScore

	calculates the total score value of a score_t object

-------------------------------------------------------------------------------*/

int totalScore(score_t* score)
{
	int amount = 0;

	node_t* node;
	for ( node = score->stats->inventory.first; node != NULL; node = node->next )
	{
		Item* item = (Item*)node->element;
		amount += items[item->type].value;
	}
	amount += score->stats->GOLD;
	amount += score->stats->EXP;
	amount += score->stats->LVL * 500;

	int c;
	for ( c = 0; c < NUMPROFICIENCIES; c++ )
	{
		amount += score->stats->PROFICIENCIES[c];
	}
	for ( c = 0; c < NUMMONSTERS; c++ )
	{
		if ( c != HUMAN )
		{
			amount += score->kills[c] * 100;
		}
		else
		{
			amount -= score->kills[c] * 100;
		}
	}

	amount += score->dungeonlevel * 500;
	if ( score->victory == 3 )
	{
		amount += score->victory * 20000;
	}
	else
	{
		amount += score->victory * 10000;
	}
	amount -= score->completionTime / TICKS_PER_SECOND;
	if ( score->victory )
	{
		amount += score->conductPenniless * 5000;
		amount += score->conductFoodless * 5000;
		amount += score->conductVegetarian * 5000;
		amount += score->conductIlliterate * 5000;
		amount += conductGameChallenges[CONDUCT_BOOTS_SPEED] * 20000;
		amount += conductGameChallenges[CONDUCT_BRAWLER] * 20000;
		amount += conductGameChallenges[CONDUCT_RANGED_ONLY] * 20000;
		amount += conductGameChallenges[CONDUCT_ACCURSED] * 50000;
		amount += conductGameChallenges[CONDUCT_BLESSED_BOOTS_SPEED] * 100000;
		if ( score->conductGameChallenges[CONDUCT_HARDCORE] == 1
			&& score->conductGameChallenges[CONDUCT_CHEATS_ENABLED] == 0 )
		{
			amount *= 2;
		}
		if ( score->conductGameChallenges[CONDUCT_KEEPINVENTORY] )
		{
			amount /= 2;
		}
		if ( score->conductGameChallenges[CONDUCT_LIFESAVING] )
		{
			amount /= 4;
		}
	}
	if ( amount < 0 )
	{
		amount = 0;
	}

	return amount;
}

/*-------------------------------------------------------------------------------

	loadScore

	loads the given highscore into stats[0] so that it may be displayed
	in a character window at the main menu

-------------------------------------------------------------------------------*/

void loadScore(int scorenum)
{
	node_t* node = nullptr;
	if ( scoreDisplayMultiplayer )
	{
		node = list_Node(&topscoresMultiplayer, scorenum);
	}
	else
	{
		node = list_Node(&topscores, scorenum);
	}
	if ( !node )
	{
		return;
	}
	score_t* score = (score_t*)node->element;
	stats[0]->clearStats();

	int c;
	for ( c = 0; c < NUMMONSTERS; c++ )
	{
		kills[c] = score->kills[c];
	}
	stats[0]->type = score->stats->type;
	stats[0]->sex = score->stats->sex;
	stats[0]->appearance = score->stats->appearance;
	stats[0]->playerRace = score->stats->playerRace;
	//((stats[0]->appearance & 0xFF00) >> 8);
	//stats[0]->appearance = (stats[0]->appearance & 0xFF);
	strcpy(stats[0]->name, score->stats->name);
	client_classes[0] = score->classnum;
	victory = score->victory;
	currentlevel = score->dungeonlevel;

	completionTime = score->completionTime;
	conductPenniless = score->conductPenniless;
	conductFoodless = score->conductFoodless;
	conductVegetarian = score->conductVegetarian;
	conductIlliterate = score->conductIlliterate;

	stats[0]->HP = score->stats->HP;
	stats[0]->MAXHP = score->stats->MAXHP;
	stats[0]->MP = score->stats->MP;
	stats[0]->MAXMP = score->stats->MAXMP;
	stats[0]->STR = score->stats->STR;
	stats[0]->DEX = score->stats->DEX;
	stats[0]->CON = score->stats->CON;
	stats[0]->INT = score->stats->INT;
	stats[0]->PER = score->stats->PER;
	stats[0]->CHR = score->stats->CHR;
	stats[0]->EXP = score->stats->EXP;
	stats[0]->LVL = score->stats->LVL;
	stats[0]->GOLD = score->stats->GOLD;
	stats[0]->HUNGER = score->stats->HUNGER;
	for ( c = 0; c < NUMPROFICIENCIES; c++ )
	{
		stats[0]->PROFICIENCIES[c] = score->stats->PROFICIENCIES[c];
	}
	for ( c = 0; c < NUMEFFECTS; c++ )
	{
		stats[0]->EFFECTS[c] = score->stats->EFFECTS[c];
		stats[0]->EFFECTS_TIMERS[c] = score->stats->EFFECTS_TIMERS[c];
	}
	list_FreeAll(&stats[0]->inventory);
	list_Copy(&stats[0]->inventory, &score->stats->inventory);
	for ( node = stats[0]->inventory.first; node != NULL; node = node->next )
	{
		Item* item = (Item*)node->element;
		item->node = node;
	}
	for ( c = 0, node = score->stats->inventory.first; node != NULL; node = node->next, c++ )
	{
		Item* item = (Item*)node->element;
		if ( score->stats->helmet == item )
		{
			node_t* node2 = list_Node(&stats[0]->inventory, c);
			Item* item2 = (Item*)node2->element;
			stats[0]->helmet = item2;
		}
		else if ( score->stats->breastplate == item )
		{
			node_t* node2 = list_Node(&stats[0]->inventory, c);
			Item* item2 = (Item*)node2->element;
			stats[0]->breastplate = item2;
		}
		else if ( score->stats->gloves == item )
		{
			node_t* node2 = list_Node(&stats[0]->inventory, c);
			Item* item2 = (Item*)node2->element;
			stats[0]->gloves = item2;
		}
		else if ( score->stats->shoes == item )
		{
			node_t* node2 = list_Node(&stats[0]->inventory, c);
			Item* item2 = (Item*)node2->element;
			stats[0]->shoes = item2;
		}
		else if ( score->stats->shield == item )
		{
			node_t* node2 = list_Node(&stats[0]->inventory, c);
			Item* item2 = (Item*)node2->element;
			stats[0]->shield = item2;
		}
		else if ( score->stats->weapon == item )
		{
			node_t* node2 = list_Node(&stats[0]->inventory, c);
			Item* item2 = (Item*)node2->element;
			stats[0]->weapon = item2;
		}
		else if ( score->stats->cloak == item )
		{
			node_t* node2 = list_Node(&stats[0]->inventory, c);
			Item* item2 = (Item*)node2->element;
			stats[0]->cloak = item2;
		}
		else if ( score->stats->amulet == item )
		{
			node_t* node2 = list_Node(&stats[0]->inventory, c);
			Item* item2 = (Item*)node2->element;
			stats[0]->amulet = item2;
		}
		else if ( score->stats->ring == item )
		{
			node_t* node2 = list_Node(&stats[0]->inventory, c);
			Item* item2 = (Item*)node2->element;
			stats[0]->ring = item2;
		}
		else if ( score->stats->mask == item )
		{
			node_t* node2 = list_Node(&stats[0]->inventory, c);
			Item* item2 = (Item*)node2->element;
			stats[0]->mask = item2;
		}
	}
	for ( c = 0; c < NUM_CONDUCT_CHALLENGES; ++c )
	{
		conductGameChallenges[c] = score->conductGameChallenges[c];
	}
	for ( c = 0; c < NUM_GAMEPLAY_STATISTICS; ++c )
	{
		gameStatistics[c] = score->gameStatistics[c];
	}
}

/*-------------------------------------------------------------------------------

	saveAllScores

	saves all highscores to the scores data file

-------------------------------------------------------------------------------*/

void saveAllScores(const std::string& scoresfilename)
{
	node_t* node;
	FILE* fp;
	int c;

	char path[PATH_MAX] = "";
	completePath(path, scoresfilename.c_str(), outputdir);

	// open file
	if ( (fp = fopen(path, "wb")) == NULL )
	{
		printlog("error: failed to save '%s!'\n", scoresfilename.c_str());
		return;
	}

	// magic number
	fprintf(fp, "BARONYSCORES");
	fprintf(fp, VERSION);

	// header info
	c = list_Size(&booksRead);
	fwrite(&c, sizeof(Uint32), 1, fp);
	for ( node = booksRead.first; node != NULL; node = node->next )
	{
		char* book = (char*)node->element;
		c = strlen(book);
		fwrite(&c, sizeof(Uint32), 1, fp);
		fputs(book, fp);
	}
	for ( c = 0; c < NUMCLASSES; c++ )
	{
		fwrite(&usedClass[c], sizeof(bool), 1, fp);
	}
	for ( c = 0; c < NUMRACES; c++ )
	{
		fwrite(&usedRace[c], sizeof(bool), 1, fp);
	}

	// score list
	if ( scoresfilename.compare(SCORESFILE) == 0 )
	{
		c = list_Size(&topscores);
		node = topscores.first;
	}
	else
	{
		c = list_Size(&topscoresMultiplayer);
		node = topscoresMultiplayer.first;
	}
	fwrite(&c, sizeof(Uint32), 1, fp);
	for (; node != NULL; node = node->next )
	{
		score_t* score = (score_t*)node->element;
		for ( c = 0; c < NUMMONSTERS; c++ )
		{
			fwrite(&score->kills[c], sizeof(Sint32), 1, fp);
		}
		fwrite(&score->completionTime, sizeof(Uint32), 1, fp);
		fwrite(&score->conductPenniless, sizeof(bool), 1, fp);
		fwrite(&score->conductFoodless, sizeof(bool), 1, fp);
		fwrite(&score->conductVegetarian, sizeof(bool), 1, fp);
		fwrite(&score->conductIlliterate, sizeof(bool), 1, fp);
		fwrite(&score->stats->type, sizeof(Monster), 1, fp);
		fwrite(&score->stats->sex, sizeof(sex_t), 1, fp);
		Uint32 raceAndAppearance = 0;
		raceAndAppearance |= (score->stats->playerRace << 8);
		raceAndAppearance |= (score->stats->appearance);
		fwrite(&raceAndAppearance, sizeof(Uint32), 1, fp);
		fwrite(score->stats->name, sizeof(char), 32, fp);
		fwrite(&score->classnum, sizeof(Sint32), 1, fp);
		fwrite(&score->dungeonlevel, sizeof(Sint32), 1, fp);
		fwrite(&score->victory, sizeof(int), 1, fp);
		fwrite(&score->stats->HP, sizeof(Sint32), 1, fp);
		fwrite(&score->stats->MAXHP, sizeof(Sint32), 1, fp);
		fwrite(&score->stats->MP, sizeof(Sint32), 1, fp);
		fwrite(&score->stats->MAXMP, sizeof(Sint32), 1, fp);
		fwrite(&score->stats->STR, sizeof(Sint32), 1, fp);
		fwrite(&score->stats->DEX, sizeof(Sint32), 1, fp);
		fwrite(&score->stats->CON, sizeof(Sint32), 1, fp);
		fwrite(&score->stats->INT, sizeof(Sint32), 1, fp);
		fwrite(&score->stats->PER, sizeof(Sint32), 1, fp);
		fwrite(&score->stats->CHR, sizeof(Sint32), 1, fp);
		fwrite(&score->stats->EXP, sizeof(Sint32), 1, fp);
		fwrite(&score->stats->LVL, sizeof(Sint32), 1, fp);
		fwrite(&score->stats->GOLD, sizeof(Sint32), 1, fp);
		fwrite(&score->stats->HUNGER, sizeof(Sint32), 1, fp);
		for ( c = 0; c < NUMPROFICIENCIES; c++ )
		{
			fwrite(&score->stats->PROFICIENCIES[c], sizeof(Sint32), 1, fp);
		}
		for ( c = 0; c < NUMEFFECTS; c++ )
		{
			fwrite(&score->stats->EFFECTS[c], sizeof(bool), 1, fp);
			fwrite(&score->stats->EFFECTS_TIMERS[c], sizeof(Sint32), 1, fp);
		}
		for ( c = 0; c < NUM_CONDUCT_CHALLENGES; ++c )
		{
			fwrite(&score->conductGameChallenges[c], sizeof(Sint32), 1, fp);
		}
		for ( c = 0; c < NUM_GAMEPLAY_STATISTICS; ++c )
		{
			fwrite(&score->gameStatistics[c], sizeof(Sint32), 1, fp);
		}

		// inventory
		node_t* node2;
		c = list_Size(&score->stats->inventory);
		fwrite(&c, sizeof(ItemType), 1, fp);
		for ( node2 = score->stats->inventory.first; node2 != NULL; node2 = node2->next )
		{
			Item* item = (Item*)node2->element;
			fwrite(&item->type, sizeof(ItemType), 1, fp);
			fwrite(&item->status, sizeof(Status), 1, fp);
			fwrite(&item->beatitude, sizeof(Sint16), 1, fp);
			fwrite(&item->count, sizeof(Sint16), 1, fp);
			fwrite(&item->appearance, sizeof(Uint32), 1, fp);
			fwrite(&item->identified, sizeof(bool), 1, fp);
		}
		if ( score->stats->helmet )
		{
			c = list_Index(score->stats->helmet->node);
			fwrite(&c, sizeof(ItemType), 1, fp);
		}
		else
		{
			c = list_Size(&score->stats->inventory);
			fwrite(&c, sizeof(ItemType), 1, fp);
		}
		if ( score->stats->breastplate )
		{
			c = list_Index(score->stats->breastplate->node);
			fwrite(&c, sizeof(ItemType), 1, fp);
		}
		else
		{
			c = list_Size(&score->stats->inventory);
			fwrite(&c, sizeof(ItemType), 1, fp);
		}
		if ( score->stats->gloves )
		{
			c = list_Index(score->stats->gloves->node);
			fwrite(&c, sizeof(ItemType), 1, fp);
		}
		else
		{
			c = list_Size(&score->stats->inventory);
			fwrite(&c, sizeof(ItemType), 1, fp);
		}
		if ( score->stats->shoes )
		{
			c = list_Index(score->stats->shoes->node);
			fwrite(&c, sizeof(ItemType), 1, fp);
		}
		else
		{
			c = list_Size(&score->stats->inventory);
			fwrite(&c, sizeof(ItemType), 1, fp);
		}
		if ( score->stats->shield )
		{
			c = list_Index(score->stats->shield->node);
			fwrite(&c, sizeof(ItemType), 1, fp);
		}
		else
		{
			c = list_Size(&score->stats->inventory);
			fwrite(&c, sizeof(ItemType), 1, fp);
		}
		if ( score->stats->weapon )
		{
			c = list_Index(score->stats->weapon->node);
			fwrite(&c, sizeof(ItemType), 1, fp);
		}
		else
		{
			c = list_Size(&score->stats->inventory);
			fwrite(&c, sizeof(ItemType), 1, fp);
		}
		if ( score->stats->cloak )
		{
			c = list_Index(score->stats->cloak->node);
			fwrite(&c, sizeof(ItemType), 1, fp);
		}
		else
		{
			c = list_Size(&score->stats->inventory);
			fwrite(&c, sizeof(ItemType), 1, fp);
		}
		if ( score->stats->amulet )
		{
			c = list_Index(score->stats->amulet->node);
			fwrite(&c, sizeof(ItemType), 1, fp);
		}
		else
		{
			c = list_Size(&score->stats->inventory);
			fwrite(&c, sizeof(ItemType), 1, fp);
		}
		if ( score->stats->ring )
		{
			c = list_Index(score->stats->ring->node);
			fwrite(&c, sizeof(ItemType), 1, fp);
		}
		else
		{
			c = list_Size(&score->stats->inventory);
			fwrite(&c, sizeof(ItemType), 1, fp);
		}
		if ( score->stats->mask )
		{
			c = list_Index(score->stats->mask->node);
			fwrite(&c, sizeof(ItemType), 1, fp);
		}
		else
		{
			c = list_Size(&score->stats->inventory);
			fwrite(&c, sizeof(ItemType), 1, fp);
		}
	}

	fclose(fp);
}

/*-------------------------------------------------------------------------------

	loadAllScores

	loads all highscores from the scores data file

-------------------------------------------------------------------------------*/

void loadAllScores(const std::string& scoresfilename)
{
	FILE* fp;
	Uint32 c, i;
	char path[PATH_MAX] = "";
	completePath(path, scoresfilename.c_str(), outputdir);

	// clear top scores
	if ( scoresfilename.compare(SCORESFILE) == 0 )
	{
		list_FreeAll(&topscores);
	}
	else
	{
		list_FreeAll(&topscoresMultiplayer);
	}

	// open file
	if ( (fp = fopen(path, "rb")) == NULL )
	{
		return;
	}

	// magic number
	char checkstr[64];
	fread(checkstr, sizeof(char), strlen("BARONYSCORES"), fp);
	if ( strncmp(checkstr, "BARONYSCORES", strlen("BARONYSCORES")) )
	{
		printlog("error: '%s' is corrupt!\n", scoresfilename.c_str());
		fclose(fp);
		return;
	}

	fread(checkstr, sizeof(char), strlen(VERSION), fp);

	int versionNumber = 300;
	char versionStr[4] = "000";
	i = 0;
	for ( int j = 0; j < strlen(VERSION); ++j )
	{
		if ( checkstr[j] >= '0' && checkstr[j] <= '9' )
		{
			versionStr[i] = checkstr[j]; // copy all integers into versionStr.
			++i;
			if ( i == 3 )
			{
				versionStr[i] = '\0';
				break; // written 3 characters, add termination and break loop.
			}
		}
	}
	versionNumber = atoi(versionStr); // convert from string to int.
	printlog("notice: '%s' version number %d", scoresfilename.c_str(), versionNumber);
	if ( versionNumber < 200 || versionNumber > 999 )
	{
		// if version number less than v2.0.0, or more than 3 digits, abort and rebuild scores file.
		printlog("error: '%s' is corrupt!\n", scoresfilename.c_str());
		fclose(fp);
		return;
	}

	// header info
	list_FreeAll(&booksRead);
	fread(&c, sizeof(Uint32), 1, fp);
	for ( i = 0; i < c; i++ )
	{
		Uint32 booknamelen = 0;
		fread(&booknamelen, sizeof(Uint32), 1, fp);
		fgets(tempstr, booknamelen + 1, fp);

		char* book = (char*) malloc(sizeof(char) * (strlen(tempstr) + 1));
		strcpy(book, tempstr);

		node_t* node = list_AddNodeLast(&booksRead);
		node->element = book;
		node->size = sizeof(char) * (strlen(tempstr) + 1);
		node->deconstructor = &defaultDeconstructor;
	}
	for ( c = 0; c < NUMCLASSES; c++ )
	{
		if ( versionNumber < 300 )
		{
			if ( c < 10 )
			{
				fread(&usedClass[c], sizeof(bool), 1, fp);
			}
			else
			{
				usedClass[c] = false;
			}
		}
		else if ( versionNumber < 323 )
		{
			if ( c < 13 )
			{
				fread(&usedClass[c], sizeof(bool), 1, fp);
			}
			else
			{
				usedClass[c] = false;
			}
		}
		else
		{
			fread(&usedClass[c], sizeof(bool), 1, fp);
		}
	}

	for ( c = 0; c < NUMRACES; c++ )
	{
		if ( versionNumber <= 325 )
		{
			// don't read race info.
			usedRace[c] = false;
		}
		else
		{
			fread(&usedRace[c], sizeof(bool), 1, fp);
		}
	}

	// read scores
	Uint32 numscores = 0;
	fread(&numscores, sizeof(Uint32), 1, fp);
	for ( i = 0; i < numscores; i++ )
	{
		node_t* node = nullptr;
		if ( scoresfilename.compare(SCORESFILE) == 0 )
		{
			node = list_AddNodeLast(&topscores);
		}
		else
		{
			node = list_AddNodeLast(&topscoresMultiplayer);
		}
		score_t* score = (score_t*) malloc(sizeof(score_t));
		if ( !score )
		{
			printlog( "failed to allocate memory for new score!\n" );
			exit(1);
		}
		// Stat set to 0 as monster type not needed, values will be overwritten by the savegame data
		score->stats = new Stat(0);
		if ( !score->stats )
		{
			printlog( "failed to allocate memory for new stat!\n" );
			exit(1);
		}
		node->element = score;
		node->deconstructor = &scoreDeconstructor;
		node->size = sizeof(score_t);

		if ( versionNumber < 300 )
		{
			// legacy nummonsters
			for ( c = 0; c < NUMMONSTERS; c++ )
			{
				if ( c < 21 )
				{
					fread(&score->kills[c], sizeof(Sint32), 1, fp);
				}
				else
				{
					score->kills[c] = 0;
				}
			}
		}
		else if ( versionNumber < 325 )
		{
			// legacy nummonsters
			for ( c = 0; c < NUMMONSTERS; c++ )
			{
				if ( c < 33 )
				{
					fread(&score->kills[c], sizeof(Sint32), 1, fp);
				}
				else
				{
					score->kills[c] = 0;
				}
			}
		}
		else
		{
			for ( c = 0; c < NUMMONSTERS; c++ )
			{
				fread(&score->kills[c], sizeof(Sint32), 1, fp);
			}
		}
		fread(&score->completionTime, sizeof(Uint32), 1, fp);
		fread(&score->conductPenniless, sizeof(bool), 1, fp);
		fread(&score->conductFoodless, sizeof(bool), 1, fp);
		fread(&score->conductVegetarian, sizeof(bool), 1, fp);
		fread(&score->conductIlliterate, sizeof(bool), 1, fp);
		fread(&score->stats->type, sizeof(Monster), 1, fp);
		fread(&score->stats->sex, sizeof(sex_t), 1, fp);
		fread(&score->stats->appearance, sizeof(Uint32), 1, fp);
		if ( versionNumber >= 323 )
		{
			score->stats->playerRace = ((score->stats->appearance & 0xFF00) >> 8);
			score->stats->appearance = (score->stats->appearance & 0xFF);
		}
		fread(&score->stats->name, sizeof(char), 32, fp);
		fread(&score->classnum, sizeof(Sint32), 1, fp);
		fread(&score->dungeonlevel, sizeof(Sint32), 1, fp);
		fread(&score->victory, sizeof(int), 1, fp);
		actualScoreVictory = std::max(actualScoreVictory, score->victory);
		fread(&score->stats->HP, sizeof(Sint32), 1, fp);
		fread(&score->stats->MAXHP, sizeof(Sint32), 1, fp);
		fread(&score->stats->MP, sizeof(Sint32), 1, fp);
		fread(&score->stats->MAXMP, sizeof(Sint32), 1, fp);
		fread(&score->stats->STR, sizeof(Sint32), 1, fp);
		fread(&score->stats->DEX, sizeof(Sint32), 1, fp);
		fread(&score->stats->CON, sizeof(Sint32), 1, fp);
		fread(&score->stats->INT, sizeof(Sint32), 1, fp);
		fread(&score->stats->PER, sizeof(Sint32), 1, fp);
		fread(&score->stats->CHR, sizeof(Sint32), 1, fp);
		fread(&score->stats->EXP, sizeof(Sint32), 1, fp);
		fread(&score->stats->LVL, sizeof(Sint32), 1, fp);
		fread(&score->stats->GOLD, sizeof(Sint32), 1, fp);
		fread(&score->stats->HUNGER, sizeof(Sint32), 1, fp);
		for ( c = 0; c < NUMPROFICIENCIES; c++ )
		{
			if ( versionNumber < 323 && c >= PRO_UNARMED )
			{
				score->stats->PROFICIENCIES[c] = 0;
			}
			else
			{
				fread(&score->stats->PROFICIENCIES[c], sizeof(Sint32), 1, fp);
			}
		}
		if ( versionNumber < 300 )
		{
			// legacy effects
			for ( c = 0; c < NUMEFFECTS; c++ )
			{
				if ( c < 16 )
				{
					fread(&score->stats->EFFECTS[c], sizeof(bool), 1, fp);
					fread(&score->stats->EFFECTS_TIMERS[c], sizeof(Sint32), 1, fp);
				}
				else
				{
					score->stats->EFFECTS[c] = false;
					score->stats->EFFECTS_TIMERS[c] = 0;
				}
			}
		}
		else if ( versionNumber < 302 )
		{
			for ( c = 0; c < NUMEFFECTS; c++ )
			{
				if ( c < 19 )
				{
					fread(&score->stats->EFFECTS[c], sizeof(bool), 1, fp);
					fread(&score->stats->EFFECTS_TIMERS[c], sizeof(Sint32), 1, fp);
				}
				else
				{
					score->stats->EFFECTS[c] = false;
					score->stats->EFFECTS_TIMERS[c] = 0;
				}
			}
		}
		else if ( versionNumber <= 323 )
		{
			for ( c = 0; c < NUMEFFECTS; c++ )
			{
				if ( c < 32 )
				{
					fread(&score->stats->EFFECTS[c], sizeof(bool), 1, fp);
					fread(&score->stats->EFFECTS_TIMERS[c], sizeof(Sint32), 1, fp);
				}
				else
				{
					score->stats->EFFECTS[c] = false;
					score->stats->EFFECTS_TIMERS[c] = 0;
				}
			}
		}
		else
		{
			for ( c = 0; c < NUMEFFECTS; c++ )
			{
				fread(&score->stats->EFFECTS[c], sizeof(bool), 1, fp);
				fread(&score->stats->EFFECTS_TIMERS[c], sizeof(Sint32), 1, fp);
			}
		}

		if ( versionNumber >= 310 )
		{
			for ( c = 0; c < NUM_CONDUCT_CHALLENGES; ++c )
			{
				fread(&score->conductGameChallenges[c], sizeof(Sint32), 1, fp);
			}
			for ( c = 0; c < NUM_GAMEPLAY_STATISTICS; ++c )
			{
				fread(&score->gameStatistics[c], sizeof(Sint32), 1, fp);
			}
		}
		else
		{
			for ( c = 0; c < NUM_CONDUCT_CHALLENGES; ++c )
			{
				score->conductGameChallenges[c] = 0;
			}
		}
		score->stats->leader_uid = 0;
		score->stats->FOLLOWERS.first = NULL;
		score->stats->FOLLOWERS.last = NULL;
		score->stats->stache_x1 = 0;
		score->stats->stache_x2 = 0;
		score->stats->stache_y1 = 0;
		score->stats->stache_y2 = 0;

		// inventory
		int numitems = 0;
		fread(&numitems, sizeof(Uint32), 1, fp);
		score->stats->inventory.first = NULL;
		score->stats->inventory.last = NULL;
		for ( c = 0; c < numitems; c++ )
		{
			ItemType type;
			Status status;
			Sint16 beatitude;
			Sint16 count;
			Uint32 appearance;
			bool identified;
			fread(&type, sizeof(ItemType), 1, fp);
			fread(&status, sizeof(Status), 1, fp);
			fread(&beatitude, sizeof(Sint16), 1, fp);
			fread(&count, sizeof(Sint16), 1, fp);
			fread(&appearance, sizeof(Uint32), 1, fp);
			fread(&identified, sizeof(bool), 1, fp);
			newItem(type, status, beatitude, count, appearance, identified, &score->stats->inventory);
		}
		fread(&c, sizeof(Uint32), 1, fp);
		node = list_Node(&score->stats->inventory, c);
		if ( node )
		{
			score->stats->helmet = (Item*)node->element;
		}
		else
		{
			score->stats->helmet = NULL;
		}
		fread(&c, sizeof(Uint32), 1, fp);
		node = list_Node(&score->stats->inventory, c);
		if ( node )
		{
			score->stats->breastplate = (Item*)node->element;
		}
		else
		{
			score->stats->breastplate = NULL;
		}
		fread(&c, sizeof(Uint32), 1, fp);
		node = list_Node(&score->stats->inventory, c);
		if ( node )
		{
			score->stats->gloves = (Item*)node->element;
		}
		else
		{
			score->stats->gloves = NULL;
		}
		fread(&c, sizeof(Uint32), 1, fp);
		node = list_Node(&score->stats->inventory, c);
		if ( node )
		{
			score->stats->shoes = (Item*)node->element;
		}
		else
		{
			score->stats->shoes = NULL;
		}
		fread(&c, sizeof(Uint32), 1, fp);
		node = list_Node(&score->stats->inventory, c);
		if ( node )
		{
			score->stats->shield = (Item*)node->element;
		}
		else
		{
			score->stats->shield = NULL;
		}
		fread(&c, sizeof(Uint32), 1, fp);
		node = list_Node(&score->stats->inventory, c);
		if ( node )
		{
			score->stats->weapon = (Item*)node->element;
		}
		else
		{
			score->stats->weapon = NULL;
		}
		fread(&c, sizeof(Uint32), 1, fp);
		node = list_Node(&score->stats->inventory, c);
		if ( node )
		{
			score->stats->cloak = (Item*)node->element;
		}
		else
		{
			score->stats->cloak = NULL;
		}
		fread(&c, sizeof(Uint32), 1, fp);
		node = list_Node(&score->stats->inventory, c);
		if ( node )
		{
			score->stats->amulet = (Item*)node->element;
		}
		else
		{
			score->stats->amulet = NULL;
		}
		fread(&c, sizeof(Uint32), 1, fp);
		node = list_Node(&score->stats->inventory, c);
		if ( node )
		{
			score->stats->ring = (Item*)node->element;
		}
		else
		{
			score->stats->ring = NULL;
		}
		fread(&c, sizeof(Uint32), 1, fp);
		node = list_Node(&score->stats->inventory, c);
		if ( node )
		{
			score->stats->mask = (Item*)node->element;
		}
		else
		{
			score->stats->mask = NULL;
		}

		score->stats->monster_sound = NULL;
		score->stats->monster_idlevar = 0;
	}

	fclose(fp);
}

/*-------------------------------------------------------------------------------

	saveGame

	Saves the player character as they were at the start of the
	last level

-------------------------------------------------------------------------------*/

int saveGame(int saveIndex)
{
	if ( gameModeManager.getMode() != GameModeManager_t::GAME_MODE_DEFAULT )
	{
		return 1;
	}

	int player;
	node_t* node;
	FILE* fp;
	Sint32 c;
	char savefile[PATH_MAX] = "";
	char path[PATH_MAX] = "";

	// open file
	if ( !intro )
	{
		messagePlayer(clientnum, language[1121]);
	}

	if ( multiplayer == SINGLE )
	{
		strncpy(savefile, setSaveGameFileName(true, false, saveIndex).c_str(), PATH_MAX - 1);
	}
	else
	{
		strncpy(savefile, setSaveGameFileName(false, false, saveIndex).c_str(), PATH_MAX - 1);
	}
	completePath(path, savefile, outputdir);

	if ( (fp = fopen(path, "wb")) == NULL )
	{
		printlog("warning: failed to save '%s'!\n", path);
		return 1;
	}

	// write header info
	fprintf(fp, "BARONYSAVEGAME");
	fprintf(fp, VERSION);
	fwrite(&uniqueGameKey, sizeof(Uint32), 1, fp);
	if ( multiplayer > SINGLE && directConnect )
	{
		multiplayer += 2;
		fwrite(&multiplayer, sizeof(Uint32), 1, fp);
		multiplayer -= 2;
	}
	else
	{
		if ( multiplayer == SERVER && LobbyHandler.hostingType == LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY )
		{
			multiplayer = SERVERCROSSPLAY;
			fwrite(&multiplayer, sizeof(Uint32), 1, fp);
			multiplayer = SERVER;
		}
		else
		{
			fwrite(&multiplayer, sizeof(Uint32), 1, fp);
		}
	}
	Uint32 hash = 0;
#ifdef WINDOWS
	struct _stat result;
	if ( _stat(path, &result) == 0 )
	{
		struct tm *tm = localtime(&result.st_mtime);
		if ( tm )
		{
			hash = tm->tm_hour + tm->tm_mday * tm->tm_year + tm->tm_wday + tm->tm_yday;
		}
	}
#else
	struct stat result;
	if ( stat(path, &result) == 0 )
	{
		struct tm *tm = localtime(&result.st_mtime);
		if ( tm )
		{
			hash = tm->tm_hour + tm->tm_mday * tm->tm_year + tm->tm_wday + tm->tm_yday;
		}
	}
#endif // WINDOWS
	hash += (stats[clientnum]->STR + stats[clientnum]->LVL + stats[clientnum]->DEX * stats[clientnum]->INT);
	hash += (stats[clientnum]->CON * stats[clientnum]->PER + std::min(stats[clientnum]->GOLD, 5000) - stats[clientnum]->CON);
	hash += (stats[clientnum]->HP - stats[clientnum]->MP);
	hash += (currentlevel);
	Uint32 writeCurrentLevel = (hash << 8);
	writeCurrentLevel |= (currentlevel & 0xFF);

	fwrite(&clientnum, sizeof(Uint32), 1, fp);
	fwrite(&mapseed, sizeof(Uint32), 1, fp);
	fwrite(&writeCurrentLevel, sizeof(Uint32), 1, fp);
	fwrite(&secretlevel, sizeof(bool), 1, fp);
	fwrite(&completionTime, sizeof(Uint32), 1, fp);
	fwrite(&conductPenniless, sizeof(bool), 1, fp);
	fwrite(&conductFoodless, sizeof(bool), 1, fp);
	fwrite(&conductVegetarian, sizeof(bool), 1, fp);
	fwrite(&conductIlliterate, sizeof(bool), 1, fp);
	for ( c = 0; c < NUM_CONDUCT_CHALLENGES; ++c )
	{
		fwrite(&conductGameChallenges[c], sizeof(Sint32), 1, fp);
	}
	for ( c = 0; c < NUM_GAMEPLAY_STATISTICS; ++c )
	{
		fwrite(&gameStatistics[c], sizeof(Sint32), 1, fp);
	}
	fwrite(&svFlags, sizeof(Uint32), 1, fp);

	// write hotbar items
	for ( c = 0; c < NUM_HOTBAR_SLOTS; c++ )
	{
		int index = list_Size(&stats[clientnum]->inventory);
		Item* item = uidToItem(hotbar[c].item);
		if ( item )
		{
			index = list_Index(item->node);
		}
		fwrite(&index, sizeof(Uint32), 1, fp);
	}

	// write spells
	Uint32 numspells = list_Size(&spellList);
	fwrite(&numspells, sizeof(Uint32), 1, fp);
	for ( node = spellList.first; node != NULL; node = node->next )
	{
		spell_t* spell = (spell_t*)node->element;
		fwrite(&spell->ID, sizeof(Uint32), 1, fp);
	}


	// player data
	for ( player = 0; player < MAXPLAYERS; player++ )
	{
		fwrite(&client_classes[player], sizeof(Uint32), 1, fp);
		for ( c = 0; c < NUMMONSTERS; c++ )
		{
			fwrite(&kills[c], sizeof(Sint32), 1, fp);
		}
		fwrite(&stats[player]->type, sizeof(Monster), 1, fp);
		fwrite(&stats[player]->sex, sizeof(sex_t), 1, fp);
		Uint32 raceAndAppearance = 0;
		raceAndAppearance |= (stats[player]->playerRace << 8);
		raceAndAppearance |= (stats[player]->appearance);
		fwrite(&raceAndAppearance, sizeof(Uint32), 1, fp);
		fwrite(stats[player]->name, sizeof(char), 32, fp);
		fwrite(&stats[player]->HP, sizeof(Sint32), 1, fp);
		fwrite(&stats[player]->MAXHP, sizeof(Sint32), 1, fp);
		fwrite(&stats[player]->MP, sizeof(Sint32), 1, fp);
		fwrite(&stats[player]->MAXMP, sizeof(Sint32), 1, fp);
		fwrite(&stats[player]->STR, sizeof(Sint32), 1, fp);
		fwrite(&stats[player]->DEX, sizeof(Sint32), 1, fp);
		fwrite(&stats[player]->CON, sizeof(Sint32), 1, fp);
		fwrite(&stats[player]->INT, sizeof(Sint32), 1, fp);
		fwrite(&stats[player]->PER, sizeof(Sint32), 1, fp);
		fwrite(&stats[player]->CHR, sizeof(Sint32), 1, fp);
		fwrite(&stats[player]->EXP, sizeof(Sint32), 1, fp);
		fwrite(&stats[player]->LVL, sizeof(Sint32), 1, fp);
		fwrite(&stats[player]->GOLD, sizeof(Sint32), 1, fp);
		fwrite(&stats[player]->HUNGER, sizeof(Sint32), 1, fp);
		for ( c = 0; c < NUMPROFICIENCIES; c++ )
		{
			fwrite(&stats[player]->PROFICIENCIES[c], sizeof(Sint32), 1, fp);
		}
		for ( c = 0; c < NUMEFFECTS; c++ )
		{
			fwrite(&stats[player]->EFFECTS[c], sizeof(bool), 1, fp);
			fwrite(&stats[player]->EFFECTS_TIMERS[c], sizeof(Sint32), 1, fp);
		}
		for ( c = 0; c < 32; c++ )
		{
			fwrite(&stats[player]->MISC_FLAGS[c], sizeof(Sint32), 1, fp);
		}

		// inventory
		if ( player == clientnum )
		{
			c = list_Size(&stats[player]->inventory);
			fwrite(&c, sizeof(Uint32), 1, fp);
			for ( node = stats[player]->inventory.first; node != NULL; node = node->next )
			{
				Item* item = (Item*)node->element;
				fwrite(&item->type, sizeof(ItemType), 1, fp);
				fwrite(&item->status, sizeof(Status), 1, fp);
				fwrite(&item->beatitude, sizeof(Sint16), 1, fp);
				fwrite(&item->count, sizeof(Sint16), 1, fp);
				fwrite(&item->appearance, sizeof(Uint32), 1, fp);
				fwrite(&item->identified, sizeof(bool), 1, fp);
				fwrite(&item->x, sizeof(Sint32), 1, fp);
				fwrite(&item->y, sizeof(Sint32), 1, fp);
			}
			if ( stats[player]->helmet )
			{
				c = list_Index(stats[player]->helmet->node);
				fwrite(&c, sizeof(Uint32), 1, fp);
			}
			else
			{
				c = list_Size(&stats[player]->inventory);
				fwrite(&c, sizeof(Uint32), 1, fp);
			}
			if ( stats[player]->breastplate )
			{
				c = list_Index(stats[player]->breastplate->node);
				fwrite(&c, sizeof(Uint32), 1, fp);
			}
			else
			{
				c = list_Size(&stats[player]->inventory);
				fwrite(&c, sizeof(Uint32), 1, fp);
			}
			if ( stats[player]->gloves )
			{
				c = list_Index(stats[player]->gloves->node);
				fwrite(&c, sizeof(Uint32), 1, fp);
			}
			else
			{
				c = list_Size(&stats[player]->inventory);
				fwrite(&c, sizeof(Uint32), 1, fp);
			}
			if ( stats[player]->shoes )
			{
				c = list_Index(stats[player]->shoes->node);
				fwrite(&c, sizeof(Uint32), 1, fp);
			}
			else
			{
				c = list_Size(&stats[player]->inventory);
				fwrite(&c, sizeof(Uint32), 1, fp);
			}
			if ( stats[player]->shield )
			{
				c = list_Index(stats[player]->shield->node);
				fwrite(&c, sizeof(Uint32), 1, fp);
			}
			else
			{
				c = list_Size(&stats[player]->inventory);
				fwrite(&c, sizeof(Uint32), 1, fp);
			}
			if ( stats[player]->weapon )
			{
				c = list_Index(stats[player]->weapon->node);
				fwrite(&c, sizeof(Uint32), 1, fp);
			}
			else
			{
				c = list_Size(&stats[player]->inventory);
				fwrite(&c, sizeof(Uint32), 1, fp);
			}
			if ( stats[player]->cloak )
			{
				c = list_Index(stats[player]->cloak->node);
				fwrite(&c, sizeof(Uint32), 1, fp);
			}
			else
			{
				c = list_Size(&stats[player]->inventory);
				fwrite(&c, sizeof(Uint32), 1, fp);
			}
			if ( stats[player]->amulet )
			{
				c = list_Index(stats[player]->amulet->node);
				fwrite(&c, sizeof(Uint32), 1, fp);
			}
			else
			{
				c = list_Size(&stats[player]->inventory);
				fwrite(&c, sizeof(Uint32), 1, fp);
			}
			if ( stats[player]->ring )
			{
				c = list_Index(stats[player]->ring->node);
				fwrite(&c, sizeof(Uint32), 1, fp);
			}
			else
			{
				c = list_Size(&stats[player]->inventory);
				fwrite(&c, sizeof(Uint32), 1, fp);
			}
			if ( stats[player]->mask )
			{
				c = list_Index(stats[player]->mask->node);
				fwrite(&c, sizeof(Uint32), 1, fp);
			}
			else
			{
				c = list_Size(&stats[player]->inventory);
				fwrite(&c, sizeof(Uint32), 1, fp);
			}
		}
		else
		{
			if ( multiplayer == SERVER )
			{
				if ( stats[player]->helmet )
				{
					Item* item = stats[player]->helmet;
					fwrite(&item->type, sizeof(ItemType), 1, fp);
					fwrite(&item->status, sizeof(Status), 1, fp);
					fwrite(&item->beatitude, sizeof(Sint16), 1, fp);
					fwrite(&item->count, sizeof(Sint16), 1, fp);
					fwrite(&item->appearance, sizeof(Uint32), 1, fp);
					fwrite(&item->identified, sizeof(bool), 1, fp);
				}
				else
				{
					c = NUMITEMS;
					fwrite(&c, sizeof(ItemType), 1, fp);
				}
				if ( stats[player]->breastplate )
				{
					Item* item = stats[player]->breastplate;
					fwrite(&item->type, sizeof(ItemType), 1, fp);
					fwrite(&item->status, sizeof(Status), 1, fp);
					fwrite(&item->beatitude, sizeof(Sint16), 1, fp);
					fwrite(&item->count, sizeof(Sint16), 1, fp);
					fwrite(&item->appearance, sizeof(Uint32), 1, fp);
					fwrite(&item->identified, sizeof(bool), 1, fp);
				}
				else
				{
					c = NUMITEMS;
					fwrite(&c, sizeof(ItemType), 1, fp);
				}
				if ( stats[player]->gloves )
				{
					Item* item = stats[player]->gloves;
					fwrite(&item->type, sizeof(ItemType), 1, fp);
					fwrite(&item->status, sizeof(Status), 1, fp);
					fwrite(&item->beatitude, sizeof(Sint16), 1, fp);
					fwrite(&item->count, sizeof(Sint16), 1, fp);
					fwrite(&item->appearance, sizeof(Uint32), 1, fp);
					fwrite(&item->identified, sizeof(bool), 1, fp);
				}
				else
				{
					c = NUMITEMS;
					fwrite(&c, sizeof(ItemType), 1, fp);
				}
				if ( stats[player]->shoes )
				{
					Item* item = stats[player]->shoes;
					fwrite(&item->type, sizeof(ItemType), 1, fp);
					fwrite(&item->status, sizeof(Status), 1, fp);
					fwrite(&item->beatitude, sizeof(Sint16), 1, fp);
					fwrite(&item->count, sizeof(Sint16), 1, fp);
					fwrite(&item->appearance, sizeof(Uint32), 1, fp);
					fwrite(&item->identified, sizeof(bool), 1, fp);
				}
				else
				{
					c = NUMITEMS;
					fwrite(&c, sizeof(ItemType), 1, fp);
				}
				if ( stats[player]->shield )
				{
					Item* item = stats[player]->shield;
					fwrite(&item->type, sizeof(ItemType), 1, fp);
					fwrite(&item->status, sizeof(Status), 1, fp);
					fwrite(&item->beatitude, sizeof(Sint16), 1, fp);
					fwrite(&item->count, sizeof(Sint16), 1, fp);
					fwrite(&item->appearance, sizeof(Uint32), 1, fp);
					fwrite(&item->identified, sizeof(bool), 1, fp);
				}
				else
				{
					c = NUMITEMS;
					fwrite(&c, sizeof(ItemType), 1, fp);
				}
				if ( stats[player]->weapon )
				{
					Item* item = stats[player]->weapon;
					fwrite(&item->type, sizeof(ItemType), 1, fp);
					fwrite(&item->status, sizeof(Status), 1, fp);
					fwrite(&item->beatitude, sizeof(Sint16), 1, fp);
					fwrite(&item->count, sizeof(Sint16), 1, fp);
					fwrite(&item->appearance, sizeof(Uint32), 1, fp);
					fwrite(&item->identified, sizeof(bool), 1, fp);
				}
				else
				{
					c = NUMITEMS;
					fwrite(&c, sizeof(ItemType), 1, fp);
				}
				if ( stats[player]->cloak )
				{
					Item* item = stats[player]->cloak;
					fwrite(&item->type, sizeof(ItemType), 1, fp);
					fwrite(&item->status, sizeof(Status), 1, fp);
					fwrite(&item->beatitude, sizeof(Sint16), 1, fp);
					fwrite(&item->count, sizeof(Sint16), 1, fp);
					fwrite(&item->appearance, sizeof(Uint32), 1, fp);
					fwrite(&item->identified, sizeof(bool), 1, fp);
				}
				else
				{
					c = NUMITEMS;
					fwrite(&c, sizeof(ItemType), 1, fp);
				}
				if ( stats[player]->amulet )
				{
					Item* item = stats[player]->amulet;
					fwrite(&item->type, sizeof(ItemType), 1, fp);
					fwrite(&item->status, sizeof(Status), 1, fp);
					fwrite(&item->beatitude, sizeof(Sint16), 1, fp);
					fwrite(&item->count, sizeof(Sint16), 1, fp);
					fwrite(&item->appearance, sizeof(Uint32), 1, fp);
					fwrite(&item->identified, sizeof(bool), 1, fp);
				}
				else
				{
					c = NUMITEMS;
					fwrite(&c, sizeof(ItemType), 1, fp);
				}
				if ( stats[player]->ring )
				{
					Item* item = stats[player]->ring;
					fwrite(&item->type, sizeof(ItemType), 1, fp);
					fwrite(&item->status, sizeof(Status), 1, fp);
					fwrite(&item->beatitude, sizeof(Sint16), 1, fp);
					fwrite(&item->count, sizeof(Sint16), 1, fp);
					fwrite(&item->appearance, sizeof(Uint32), 1, fp);
					fwrite(&item->identified, sizeof(bool), 1, fp);
				}
				else
				{
					c = NUMITEMS;
					fwrite(&c, sizeof(ItemType), 1, fp);
				}
				if ( stats[player]->mask )
				{
					Item* item = stats[player]->mask;
					fwrite(&item->type, sizeof(ItemType), 1, fp);
					fwrite(&item->status, sizeof(Status), 1, fp);
					fwrite(&item->beatitude, sizeof(Sint16), 1, fp);
					fwrite(&item->count, sizeof(Sint16), 1, fp);
					fwrite(&item->appearance, sizeof(Uint32), 1, fp);
					fwrite(&item->identified, sizeof(bool), 1, fp);
				}
				else
				{
					c = NUMITEMS;
					fwrite(&c, sizeof(ItemType), 1, fp);
				}
			}
			else
			{
				c = NUMITEMS;
				fwrite(&c, sizeof(ItemType), 1, fp);
			}
		}
	}
	fclose(fp);

	// clients don't save follower info
	if ( multiplayer == CLIENT )
	{
		return 0;
	}

	if ( multiplayer == SINGLE )
	{
		strncpy(savefile, setSaveGameFileName(true, true, saveIndex).c_str(), PATH_MAX - 1);
	}
	else
	{
		strncpy(savefile, setSaveGameFileName(false, true, saveIndex).c_str(), PATH_MAX - 1);
	}
	completePath(path, savefile, outputdir);

	// now we save the follower information
	if ( (fp = fopen(path, "wb")) == NULL )
	{
		printlog("warning: failed to save '%s'!\n", path);
		return 1;
	}
	fprintf(fp, "BARONYSAVEGAMEFOLLOWERS");
	fprintf(fp, VERSION);

	// write follower information
	for ( c = 0; c < MAXPLAYERS; c++ )
	{
		// record number of followers for this player
		Uint32 size = list_Size(&stats[c]->FOLLOWERS);
		fwrite(&size, sizeof(Uint32), 1, fp);

		// get followerStats
		int i;
		for ( i = 0; i < size; i++ )
		{
			node_t* node = list_Node(&stats[c]->FOLLOWERS, i);
			if ( node )
			{
				Entity* follower = uidToEntity(*((Uint32*)node->element));
				Stat* followerStats = (follower) ? follower->getStats() : NULL;
				if ( followerStats )
				{
					// record follower stats
					fwrite(&followerStats->type, sizeof(Monster), 1, fp);
					fwrite(&followerStats->sex, sizeof(sex_t), 1, fp);
					fwrite(&followerStats->appearance, sizeof(Uint32), 1, fp);
					fwrite(followerStats->name, sizeof(char), 32, fp);
					fwrite(&followerStats->HP, sizeof(Sint32), 1, fp);
					fwrite(&followerStats->MAXHP, sizeof(Sint32), 1, fp);
					fwrite(&followerStats->MP, sizeof(Sint32), 1, fp);
					fwrite(&followerStats->MAXMP, sizeof(Sint32), 1, fp);
					fwrite(&followerStats->STR, sizeof(Sint32), 1, fp);
					fwrite(&followerStats->DEX, sizeof(Sint32), 1, fp);
					fwrite(&followerStats->CON, sizeof(Sint32), 1, fp);
					fwrite(&followerStats->INT, sizeof(Sint32), 1, fp);
					fwrite(&followerStats->PER, sizeof(Sint32), 1, fp);
					fwrite(&followerStats->CHR, sizeof(Sint32), 1, fp);
					fwrite(&followerStats->EXP, sizeof(Sint32), 1, fp);
					fwrite(&followerStats->LVL, sizeof(Sint32), 1, fp);
					fwrite(&followerStats->GOLD, sizeof(Sint32), 1, fp);
					fwrite(&followerStats->HUNGER, sizeof(Sint32), 1, fp);

					int j;
					for ( j = 0; j < NUMPROFICIENCIES; j++ )
					{
						fwrite(&followerStats->PROFICIENCIES[j], sizeof(Sint32), 1, fp);
					}
					for ( j = 0; j < NUMEFFECTS; j++ )
					{
						fwrite(&followerStats->EFFECTS[j], sizeof(bool), 1, fp);
						fwrite(&followerStats->EFFECTS_TIMERS[j], sizeof(Sint32), 1, fp);
					}
					for ( j = 0; j < 32; ++j )
					{
						fwrite(&followerStats->MISC_FLAGS[j], sizeof(Sint32), 1, fp);
					}

					// record follower inventory
					Uint32 invSize = list_Size(&followerStats->inventory);
					fwrite(&invSize, sizeof(Uint32), 1, fp);
					for ( node = followerStats->inventory.first; node != NULL; node = node->next )
					{
						Item* item = (Item*)node->element;
						fwrite(&item->type, sizeof(ItemType), 1, fp);
						fwrite(&item->status, sizeof(Status), 1, fp);
						fwrite(&item->beatitude, sizeof(Sint16), 1, fp);
						fwrite(&item->count, sizeof(Sint16), 1, fp);
						fwrite(&item->appearance, sizeof(Uint32), 1, fp);
						fwrite(&item->identified, sizeof(bool), 1, fp);
						fwrite(&item->x, sizeof(Sint32), 1, fp);
						fwrite(&item->y, sizeof(Sint32), 1, fp);
					}

					// record follower equipment (since NPCs never store equipment as inventory)
					if ( followerStats->helmet )
					{
						Item* item = followerStats->helmet;
						fwrite(&item->type, sizeof(ItemType), 1, fp);
						fwrite(&item->status, sizeof(Status), 1, fp);
						fwrite(&item->beatitude, sizeof(Sint16), 1, fp);
						fwrite(&item->count, sizeof(Sint16), 1, fp);
						fwrite(&item->appearance, sizeof(Uint32), 1, fp);
						fwrite(&item->identified, sizeof(bool), 1, fp);
					}
					else
					{
						ItemType tempItem = static_cast<ItemType>(NUMITEMS);
						fwrite(&tempItem, sizeof(ItemType), 1, fp);
					}
					if ( followerStats->breastplate )
					{
						Item* item = followerStats->breastplate;
						fwrite(&item->type, sizeof(ItemType), 1, fp);
						fwrite(&item->status, sizeof(Status), 1, fp);
						fwrite(&item->beatitude, sizeof(Sint16), 1, fp);
						fwrite(&item->count, sizeof(Sint16), 1, fp);
						fwrite(&item->appearance, sizeof(Uint32), 1, fp);
						fwrite(&item->identified, sizeof(bool), 1, fp);
					}
					else
					{
						ItemType tempItem = static_cast<ItemType>(NUMITEMS);
						fwrite(&tempItem, sizeof(ItemType), 1, fp);
					}
					if ( followerStats->gloves )
					{
						Item* item = followerStats->gloves;
						fwrite(&item->type, sizeof(ItemType), 1, fp);
						fwrite(&item->status, sizeof(Status), 1, fp);
						fwrite(&item->beatitude, sizeof(Sint16), 1, fp);
						fwrite(&item->count, sizeof(Sint16), 1, fp);
						fwrite(&item->appearance, sizeof(Uint32), 1, fp);
						fwrite(&item->identified, sizeof(bool), 1, fp);
					}
					else
					{
						ItemType tempItem = static_cast<ItemType>(NUMITEMS);
						fwrite(&tempItem, sizeof(ItemType), 1, fp);
					}
					if ( followerStats->shoes )
					{
						Item* item = followerStats->shoes;
						fwrite(&item->type, sizeof(ItemType), 1, fp);
						fwrite(&item->status, sizeof(Status), 1, fp);
						fwrite(&item->beatitude, sizeof(Sint16), 1, fp);
						fwrite(&item->count, sizeof(Sint16), 1, fp);
						fwrite(&item->appearance, sizeof(Uint32), 1, fp);
						fwrite(&item->identified, sizeof(bool), 1, fp);
					}
					else
					{
						ItemType tempItem = static_cast<ItemType>(NUMITEMS);
						fwrite(&tempItem, sizeof(ItemType), 1, fp);
					}
					if ( followerStats->shield )
					{
						Item* item = followerStats->shield;
						fwrite(&item->type, sizeof(ItemType), 1, fp);
						fwrite(&item->status, sizeof(Status), 1, fp);
						fwrite(&item->beatitude, sizeof(Sint16), 1, fp);
						fwrite(&item->count, sizeof(Sint16), 1, fp);
						fwrite(&item->appearance, sizeof(Uint32), 1, fp);
						fwrite(&item->identified, sizeof(bool), 1, fp);
					}
					else
					{
						ItemType tempItem = static_cast<ItemType>(NUMITEMS);
						fwrite(&tempItem, sizeof(ItemType), 1, fp);
					}
					if ( followerStats->weapon )
					{
						Item* item = followerStats->weapon;
						fwrite(&item->type, sizeof(ItemType), 1, fp);
						fwrite(&item->status, sizeof(Status), 1, fp);
						fwrite(&item->beatitude, sizeof(Sint16), 1, fp);
						fwrite(&item->count, sizeof(Sint16), 1, fp);
						fwrite(&item->appearance, sizeof(Uint32), 1, fp);
						fwrite(&item->identified, sizeof(bool), 1, fp);
					}
					else
					{
						ItemType tempItem = static_cast<ItemType>(NUMITEMS);
						fwrite(&tempItem, sizeof(ItemType), 1, fp);
					}
					if ( followerStats->cloak )
					{
						Item* item = followerStats->cloak;
						fwrite(&item->type, sizeof(ItemType), 1, fp);
						fwrite(&item->status, sizeof(Status), 1, fp);
						fwrite(&item->beatitude, sizeof(Sint16), 1, fp);
						fwrite(&item->count, sizeof(Sint16), 1, fp);
						fwrite(&item->appearance, sizeof(Uint32), 1, fp);
						fwrite(&item->identified, sizeof(bool), 1, fp);
					}
					else
					{
						ItemType tempItem = static_cast<ItemType>(NUMITEMS);
						fwrite(&tempItem, sizeof(ItemType), 1, fp);
					}
					if ( followerStats->amulet )
					{
						Item* item = followerStats->amulet;
						fwrite(&item->type, sizeof(ItemType), 1, fp);
						fwrite(&item->status, sizeof(Status), 1, fp);
						fwrite(&item->beatitude, sizeof(Sint16), 1, fp);
						fwrite(&item->count, sizeof(Sint16), 1, fp);
						fwrite(&item->appearance, sizeof(Uint32), 1, fp);
						fwrite(&item->identified, sizeof(bool), 1, fp);
					}
					else
					{
						ItemType tempItem = static_cast<ItemType>(NUMITEMS);
						fwrite(&tempItem, sizeof(ItemType), 1, fp);
					}
					if ( followerStats->ring )
					{
						Item* item = followerStats->ring;
						fwrite(&item->type, sizeof(ItemType), 1, fp);
						fwrite(&item->status, sizeof(Status), 1, fp);
						fwrite(&item->beatitude, sizeof(Sint16), 1, fp);
						fwrite(&item->count, sizeof(Sint16), 1, fp);
						fwrite(&item->appearance, sizeof(Uint32), 1, fp);
						fwrite(&item->identified, sizeof(bool), 1, fp);
					}
					else
					{
						ItemType tempItem = static_cast<ItemType>(NUMITEMS);
						fwrite(&tempItem, sizeof(ItemType), 1, fp);
					}
					if ( followerStats->mask )
					{
						Item* item = followerStats->mask;
						fwrite(&item->type, sizeof(ItemType), 1, fp);
						fwrite(&item->status, sizeof(Status), 1, fp);
						fwrite(&item->beatitude, sizeof(Sint16), 1, fp);
						fwrite(&item->count, sizeof(Sint16), 1, fp);
						fwrite(&item->appearance, sizeof(Uint32), 1, fp);
						fwrite(&item->identified, sizeof(bool), 1, fp);
					}
					else
					{
						ItemType tempItem = static_cast<ItemType>(NUMITEMS);
						fwrite(&tempItem, sizeof(ItemType), 1, fp);
					}
				}
			}
		}
	}


	fclose(fp);
	return 0;
}

/*-------------------------------------------------------------------------------

	loadGame

	Loads a character savegame stored in SAVEGAMEFILE

-------------------------------------------------------------------------------*/

int loadGame(int player, int saveIndex)
{
	Sint32 mul;
	node_t* node;
	FILE* fp;
	int c;

	char savefile[PATH_MAX] = "";
	char path[PATH_MAX] = "";
	if ( multiplayer == SINGLE )
	{
		strncpy(savefile, setSaveGameFileName(true, false, saveIndex).c_str(), PATH_MAX - 1);
	}
	else
	{
		strncpy(savefile, setSaveGameFileName(false, false, saveIndex).c_str(), PATH_MAX - 1);
	}
	completePath(path, savefile, outputdir);

	// open file
	if ( (fp = fopen(path, "rb")) == NULL )
	{
		printlog("error: failed to load '%s'!\n", path);
		return 1;
	}

	// read from file
	char checkstr[64];
	fread(checkstr, sizeof(char), strlen("BARONYSAVEGAME"), fp);
	if ( strncmp(checkstr, "BARONYSAVEGAME", strlen("BARONYSAVEGAME")) )
	{
		printlog("error: '%s' is corrupt!\n", path);
		fclose(fp);
		return 1;
	}
	fread(checkstr, sizeof(char), strlen(VERSION), fp);
	int versionNumber = getSavegameVersion(checkstr);
	printlog("loadGame: '%s' version number %d", savefile, versionNumber);
	if ( versionNumber == -1 )
	{
		// if getSavegameVersion returned -1, abort.
		printlog("error: '%s' is corrupt!\n", path);
		fclose(fp);
		return 1;
	}

	// assemble string
	Uint32 hash = 0;
	Uint32 loadedHash = 0;
#ifdef WINDOWS
	struct _stat result;
	if ( _stat(path, &result) == 0 )
	{
		struct tm *tm = localtime(&result.st_mtime);
		if ( tm )
		{
			hash = tm->tm_hour + tm->tm_mday * tm->tm_year + tm->tm_wday + tm->tm_yday;
		}
	}
#else
	struct stat result;
	if ( stat(path, &result) == 0 )
	{
		struct tm *tm = localtime(&result.st_mtime);
		if ( tm )
		{
			hash = tm->tm_hour + tm->tm_mday * tm->tm_year + tm->tm_wday + tm->tm_yday;
		}
	}
#endif // WINDOWS

	// read basic header info
	fread(&uniqueGameKey, sizeof(Uint32), 1, fp);
	fread(&mul, sizeof(Uint32), 1, fp);
	fread(&clientnum, sizeof(Uint32), 1, fp);
	fread(&mapseed, sizeof(Uint32), 1, fp);
	fread(&currentlevel, sizeof(Uint32), 1, fp);
	if ( versionNumber >= 323 )
	{
		loadedHash = (currentlevel & 0xFFFFFF00) >> 8;
		currentlevel = currentlevel & 0xFF;
	}
	fread(&secretlevel, sizeof(bool), 1, fp);
	fread(&completionTime, sizeof(Uint32), 1, fp);
	fread(&conductPenniless, sizeof(bool), 1, fp);
	fread(&conductFoodless, sizeof(bool), 1, fp);
	fread(&conductVegetarian, sizeof(bool), 1, fp);
	fread(&conductIlliterate, sizeof(bool), 1, fp);
	if ( versionNumber >= 310 )
	{
		for ( c = 0; c < NUM_CONDUCT_CHALLENGES; ++c )
		{
			fread(&conductGameChallenges[c], sizeof(Sint32), 1, fp);
		}
		for ( c = 0; c < NUM_GAMEPLAY_STATISTICS; ++c )
		{
			fread(&gameStatistics[c], sizeof(Sint32), 1, fp);
		}
	}
	if ( versionNumber >= 335 )
	{
		gameModeManager.currentSession.saveServerFlags();
		fread(&svFlags, sizeof(Uint32), 1, fp);
		printlog("[SESSION]: Using savegame server flags");
	}

	// read hotbar item offsets
	Uint32 temp_hotbar[NUM_HOTBAR_SLOTS];
	for ( c = 0; c < NUM_HOTBAR_SLOTS; c++ )
	{
		fread(&temp_hotbar[c], sizeof(Uint32), 1, fp);
	}

	// read spells
	list_FreeAll(&spellList);
	Uint32 numspells = 0;
	fread(&numspells, sizeof(Uint32), 1, fp);
	for ( c = 0; c < numspells; c++ )
	{
		int spellnum = 0;
		fread(&spellnum, sizeof(Uint32), 1, fp);
		spell_t* spell = copySpell(getSpellFromID(spellnum));

		node = list_AddNodeLast(&spellList);
		node->element = spell;
		node->deconstructor = &spellDeconstructor;
		node->size = sizeof(spell);
	}

	int monsters = NUMMONSTERS;
	if ( versionNumber < 325 )
	{
		monsters = 33;
	}

	// skip through other player data until you get to the correct player
	for ( c = 0; c < player; c++ )
	{
		fseek(fp, sizeof(Uint32), SEEK_CUR);
		fseek(fp, monsters * sizeof(Sint32), SEEK_CUR);
		fseek(fp, sizeof(Monster), SEEK_CUR);
		fseek(fp, sizeof(sex_t), SEEK_CUR);
		fseek(fp, sizeof(Uint32), SEEK_CUR);
		fseek(fp, sizeof(char) * 32, SEEK_CUR);
		fseek(fp, sizeof(Sint32), SEEK_CUR);
		fseek(fp, sizeof(Sint32), SEEK_CUR);
		fseek(fp, sizeof(Sint32), SEEK_CUR);
		fseek(fp, sizeof(Sint32), SEEK_CUR);
		fseek(fp, sizeof(Sint32), SEEK_CUR);
		fseek(fp, sizeof(Sint32), SEEK_CUR);
		fseek(fp, sizeof(Sint32), SEEK_CUR);
		fseek(fp, sizeof(Sint32), SEEK_CUR);
		fseek(fp, sizeof(Sint32), SEEK_CUR);
		fseek(fp, sizeof(Sint32), SEEK_CUR);
		fseek(fp, sizeof(Sint32), SEEK_CUR);
		fseek(fp, sizeof(Sint32), SEEK_CUR);
		fseek(fp, sizeof(Sint32), SEEK_CUR);
		fseek(fp, sizeof(Sint32), SEEK_CUR);
		if ( versionNumber >= 323 )
		{
			fseek(fp, sizeof(Sint32)*NUMPROFICIENCIES, SEEK_CUR);
		}
		else
		{
			fseek(fp, sizeof(Sint32)*14, SEEK_CUR);
		}
		if ( versionNumber <= 323 ) // legacy
		{
			fseek(fp, sizeof(bool)*32, SEEK_CUR);
			fseek(fp, sizeof(Sint32)*32, SEEK_CUR);
		}
		else
		{
			fseek(fp, sizeof(bool)*NUMEFFECTS, SEEK_CUR);
			fseek(fp, sizeof(Sint32)*NUMEFFECTS, SEEK_CUR);
		}
		if ( versionNumber >= 323 )
		{
			fseek(fp, sizeof(Sint32)*32, SEEK_CUR); // stat flags
		}

		if ( clientnum == 0 && c != 0 )
		{
			// server needs to skip past other players' equipment
			int i;
			for ( i = 0; i < 10; i++ )
			{
				int itemtype = NUMITEMS;
				fread(&itemtype, sizeof(ItemType), 1, fp);
				if ( itemtype < NUMITEMS )
				{
					fseek(fp, sizeof(Status), SEEK_CUR);
					fseek(fp, sizeof(Sint16), SEEK_CUR);
					fseek(fp, sizeof(Sint16), SEEK_CUR);
					fseek(fp, sizeof(Uint32), SEEK_CUR);
					fseek(fp, sizeof(bool), SEEK_CUR);
				}
			}
		}
		else
		{
			if ( clientnum != 0 )
			{
				// client needs to skip the dummy byte
				fseek(fp, sizeof(Status), SEEK_CUR);
			}
			else
			{
				// server needs to skip past its inventory
				int numitems = 0;
				fread(&numitems, sizeof(Uint32), 1, fp);

				int i;
				for ( i = 0; i < numitems; i++ )
				{
					fseek(fp, sizeof(ItemType), SEEK_CUR);
					fseek(fp, sizeof(Status), SEEK_CUR);
					fseek(fp, sizeof(Sint16), SEEK_CUR);
					fseek(fp, sizeof(Sint16), SEEK_CUR);
					fseek(fp, sizeof(Uint32), SEEK_CUR);
					fseek(fp, sizeof(bool), SEEK_CUR);
					fseek(fp, sizeof(Sint32), SEEK_CUR);
					fseek(fp, sizeof(Sint32), SEEK_CUR);
				}
				fseek(fp, sizeof(Uint32) * 10, SEEK_CUR); // equipment slots
			}
		}
	}

	// read in player data
	stats[player]->clearStats();
	fread(&client_classes[player], sizeof(Uint32), 1, fp);
	for ( c = 0; c < monsters; c++ )
	{
		fread(&kills[c], sizeof(Sint32), 1, fp);
	}
	fread(&stats[player]->type, sizeof(Monster), 1, fp);
	fread(&stats[player]->sex, sizeof(sex_t), 1, fp);
	fread(&stats[player]->appearance, sizeof(Uint32), 1, fp);
	if ( versionNumber >= 323 )
	{
		stats[player]->playerRace = ((stats[player]->appearance & 0xFF00) >> 8);
		stats[player]->appearance = (stats[player]->appearance & 0xFF);
	}
	fread(&stats[player]->name, sizeof(char), 32, fp);
	fread(&stats[player]->HP, sizeof(Sint32), 1, fp);
	fread(&stats[player]->MAXHP, sizeof(Sint32), 1, fp);
	fread(&stats[player]->MP, sizeof(Sint32), 1, fp);
	fread(&stats[player]->MAXMP, sizeof(Sint32), 1, fp);
	fread(&stats[player]->STR, sizeof(Sint32), 1, fp);
	fread(&stats[player]->DEX, sizeof(Sint32), 1, fp);
	fread(&stats[player]->CON, sizeof(Sint32), 1, fp);
	fread(&stats[player]->INT, sizeof(Sint32), 1, fp);
	fread(&stats[player]->PER, sizeof(Sint32), 1, fp);
	fread(&stats[player]->CHR, sizeof(Sint32), 1, fp);
	fread(&stats[player]->EXP, sizeof(Sint32), 1, fp);
	fread(&stats[player]->LVL, sizeof(Sint32), 1, fp);
	fread(&stats[player]->GOLD, sizeof(Sint32), 1, fp);
	fread(&stats[player]->HUNGER, sizeof(Sint32), 1, fp);
	for ( c = 0; c < NUMPROFICIENCIES; c++ )
	{
		if ( versionNumber < 323 && c >= PRO_UNARMED )
		{
			stats[player]->PROFICIENCIES[c] = 0;
		}
		else
		{
			fread(&stats[player]->PROFICIENCIES[c], sizeof(Sint32), 1, fp);
		}
	}
	for ( c = 0; c < NUMEFFECTS; c++ )
	{
		if ( versionNumber <= 323 ) // legacy
		{
			if ( c < 32 )
			{
				fread(&stats[player]->EFFECTS[c], sizeof(bool), 1, fp);
				fread(&stats[player]->EFFECTS_TIMERS[c], sizeof(Sint32), 1, fp);
			}
			else
			{
				stats[player]->EFFECTS[c] = false;
				stats[player]->EFFECTS_TIMERS[c] = 0;
			}
		}
		else
		{
			fread(&stats[player]->EFFECTS[c], sizeof(bool), 1, fp);
			fread(&stats[player]->EFFECTS_TIMERS[c], sizeof(Sint32), 1, fp);
		}
	}
	if ( versionNumber >= 323 )
	{
		for ( c = 0; c < 32; c++ )
		{
			fread(&stats[player]->MISC_FLAGS[c], sizeof(Sint32), 1, fp);
			if ( c < STAT_FLAG_PLAYER_RACE )
			{
				stats[player]->MISC_FLAGS[c] = 0; // we don't really need these on load.
			}
		}
	}

	if ( player == clientnum )
	{
		// inventory
		int numitems = 0;
		fread(&numitems, sizeof(Uint32), 1, fp);
		stats[player]->inventory.first = NULL;
		stats[player]->inventory.last = NULL;
		for ( c = 0; c < numitems; c++ )
		{
			ItemType type;
			Status status;
			Sint16 beatitude;
			Sint16 count;
			Uint32 appearance;
			bool identified;
			fread(&type, sizeof(ItemType), 1, fp);
			fread(&status, sizeof(Status), 1, fp);
			fread(&beatitude, sizeof(Sint16), 1, fp);
			fread(&count, sizeof(Sint16), 1, fp);
			fread(&appearance, sizeof(Uint32), 1, fp);
			fread(&identified, sizeof(bool), 1, fp);
			Item* item = newItem(type, status, beatitude, count, appearance, identified, &stats[player]->inventory);
			fread(&item->x, sizeof(Sint32), 1, fp);
			fread(&item->y, sizeof(Sint32), 1, fp);
		}

		// equipment
		fread(&c, sizeof(Uint32), 1, fp);
		node = list_Node(&stats[player]->inventory, c);
		if ( node )
		{
			stats[player]->helmet = (Item*)node->element;
		}
		else
		{
			stats[player]->helmet = NULL;
		}
		fread(&c, sizeof(Uint32), 1, fp);
		node = list_Node(&stats[player]->inventory, c);
		if ( node )
		{
			stats[player]->breastplate = (Item*)node->element;
		}
		else
		{
			stats[player]->breastplate = NULL;
		}
		fread(&c, sizeof(Uint32), 1, fp);
		node = list_Node(&stats[player]->inventory, c);
		if ( node )
		{
			stats[player]->gloves = (Item*)node->element;
		}
		else
		{
			stats[player]->gloves = NULL;
		}
		fread(&c, sizeof(Uint32), 1, fp);
		node = list_Node(&stats[player]->inventory, c);
		if ( node )
		{
			stats[player]->shoes = (Item*)node->element;
		}
		else
		{
			stats[player]->shoes = NULL;
		}
		fread(&c, sizeof(Uint32), 1, fp);
		node = list_Node(&stats[player]->inventory, c);
		if ( node )
		{
			stats[player]->shield = (Item*)node->element;
		}
		else
		{
			stats[player]->shield = NULL;
		}
		fread(&c, sizeof(Uint32), 1, fp);
		node = list_Node(&stats[player]->inventory, c);
		if ( node )
		{
			stats[player]->weapon = (Item*)node->element;
		}
		else
		{
			stats[player]->weapon = NULL;
		}
		fread(&c, sizeof(Uint32), 1, fp);
		node = list_Node(&stats[player]->inventory, c);
		if ( node )
		{
			stats[player]->cloak = (Item*)node->element;
		}
		else
		{
			stats[player]->cloak = NULL;
		}
		fread(&c, sizeof(Uint32), 1, fp);
		node = list_Node(&stats[player]->inventory, c);
		if ( node )
		{
			stats[player]->amulet = (Item*)node->element;
		}
		else
		{
			stats[player]->amulet = NULL;
		}
		fread(&c, sizeof(Uint32), 1, fp);
		node = list_Node(&stats[player]->inventory, c);
		if ( node )
		{
			stats[player]->ring = (Item*)node->element;
		}
		else
		{
			stats[player]->ring = NULL;
		}
		fread(&c, sizeof(Uint32), 1, fp);
		node = list_Node(&stats[player]->inventory, c);
		if ( node )
		{
			stats[player]->mask = (Item*)node->element;
		}
		else
		{
			stats[player]->mask = NULL;
		}
	}
	else
	{
		stats[player]->inventory.first = NULL;
		stats[player]->inventory.last = NULL;
		stats[player]->helmet = NULL;
		stats[player]->breastplate = NULL;
		stats[player]->gloves = NULL;
		stats[player]->shoes = NULL;
		stats[player]->shield = NULL;
		stats[player]->weapon = NULL;
		stats[player]->cloak = NULL;
		stats[player]->amulet = NULL;
		stats[player]->ring = NULL;
		stats[player]->mask = NULL;

		if ( multiplayer == SERVER )
		{
			for ( c = 0; c < 10; c++ )
			{
				ItemType type;
				Status status;
				Sint16 beatitude;
				Sint16 count;
				Uint32 appearance;
				bool identified;

				fread(&type, sizeof(ItemType), 1, fp);
				if ( (int)type < NUMITEMS )
				{
					fread(&status, sizeof(Status), 1, fp);
					fread(&beatitude, sizeof(Sint16), 1, fp);
					fread(&count, sizeof(Sint16), 1, fp);
					fread(&appearance, sizeof(Uint32), 1, fp);
					fread(&identified, sizeof(bool), 1, fp);

					Item* item = newItem(type, status, beatitude, count, appearance, identified, NULL);

					switch ( c )
					{
						case 0:
							stats[player]->helmet = item;
							break;
						case 1:
							stats[player]->breastplate = item;
							break;
						case 2:
							stats[player]->gloves = item;
							break;
						case 3:
							stats[player]->shoes = item;
							break;
						case 4:
							stats[player]->shield = item;
							break;
						case 5:
							stats[player]->weapon = item;
							break;
						case 6:
							stats[player]->cloak = item;
							break;
						case 7:
							stats[player]->amulet = item;
							break;
						case 8:
							stats[player]->ring = item;
							break;
						case 9:
							stats[player]->mask = item;
							break;
					}
				}
			}
		}
	}

	// assign hotbar items
	for ( c = 0; c < NUM_HOTBAR_SLOTS; c++ )
	{
		node = list_Node(&stats[player]->inventory, temp_hotbar[c]);
		if ( node )
		{
			Item* item = (Item*)node->element;
			hotbar[c].item = item->uid;
		}
		else
		{
			hotbar[c].item = 0;
			for ( int d = 0; d < NUM_HOTBAR_ALTERNATES; ++d )
			{
				hotbar_alternate[d][c].item = 0;
			}
		}
	}

	// reset some unused variables
	stats[player]->monster_sound = NULL;
	stats[player]->monster_idlevar = 0;
	stats[player]->leader_uid = 0;
	stats[player]->FOLLOWERS.first = NULL;
	stats[player]->FOLLOWERS.last = NULL;
	stats[player]->stache_x1 = 0;
	stats[player]->stache_x2 = 0;
	stats[player]->stache_y1 = 0;
	stats[player]->stache_y2 = 0;


	hash += (stats[clientnum]->STR + stats[clientnum]->LVL + stats[clientnum]->DEX * stats[clientnum]->INT);
	hash += (stats[clientnum]->CON * stats[clientnum]->PER + std::min(stats[clientnum]->GOLD, 5000) - stats[clientnum]->CON);
	hash += (stats[clientnum]->HP - stats[clientnum]->MP);
	hash += (currentlevel);

	if ( hash != loadedHash )
	{
		gameStatistics[STATISTICS_DISABLE_UPLOAD] = 1;
	}
	//printlog("%d, %d", hash, loadedHash);

	enchantedFeatherScrollSeed.seed(uniqueGameKey);
	enchantedFeatherScrollsShuffled.clear();
	enchantedFeatherScrollsShuffled = enchantedFeatherScrollsFixedList;
	std::shuffle(enchantedFeatherScrollsShuffled.begin(), enchantedFeatherScrollsShuffled.end(), enchantedFeatherScrollSeed);
	for ( auto it = enchantedFeatherScrollsShuffled.begin(); it != enchantedFeatherScrollsShuffled.end(); ++it )
	{
		//printlog("Sequence: %d", *it);
	}

	fclose(fp);
	return 0;
}

/*-------------------------------------------------------------------------------

	loadGameFollowers

	Loads follower data from a save game file

-------------------------------------------------------------------------------*/

list_t* loadGameFollowers(int saveIndex)
{
	FILE* fp;
	int c;

	char savefile[PATH_MAX] = "";
	char path[PATH_MAX] = "";
	if ( multiplayer == SINGLE )
	{
		strncpy(savefile, setSaveGameFileName(true, true, saveIndex).c_str(), PATH_MAX - 1);
	}
	else
	{
		strncpy(savefile, setSaveGameFileName(false, true, saveIndex).c_str(), PATH_MAX - 1);
	}
	completePath(path, savefile, outputdir);

	// open file
	if ( (fp = fopen(path, "rb")) == NULL )
	{
		printlog("error: failed to load '%s'!\n", path);
		return NULL;
	}

	// read from file
	char checkstr[64];
	fread(checkstr, sizeof(char), strlen("BARONYSAVEGAMEFOLLOWERS"), fp);
	if ( strncmp(checkstr, "BARONYSAVEGAMEFOLLOWERS", strlen("BARONYSAVEGAMEFOLLOWERS")) )
	{
		printlog("error: '%s' is corrupt!\n", path);
		fclose(fp);
		return NULL;
	}
	fread(checkstr, sizeof(char), strlen(VERSION), fp);
	int versionNumber = getSavegameVersion(checkstr);
	printlog("loadGameFollowers: '%s' version number %d", savefile, versionNumber);
	if ( versionNumber == -1 )
	{
		// if version number returned is invalid, abort
		printlog("error: '%s' is corrupt!\n", path);
		fclose(fp);
		return nullptr;
	}

	// create followers list
	list_t* followers = (list_t*) malloc(sizeof(list_t));
	followers->first = NULL;
	followers->last = NULL;

	// read the follower data
	for ( c = 0; c < MAXPLAYERS; c++ )
	{
		list_t* followerList = (list_t*) malloc(sizeof(list_t));
		followerList->first = NULL;
		followerList->last = NULL;
		node_t* node = list_AddNodeLast(followers);
		node->element = followerList;
		node->deconstructor = &listDeconstructor;
		node->size = sizeof(list_t);

		// number of followers for this player
		Uint32 numFollowers = 0;
		fread(&numFollowers, sizeof(Uint32), 1, fp);

		int i;
		for ( i = 0; i < numFollowers; i++ )
		{
			// Stat set to 0 as monster type not needed, values will be overwritten by the saved follower data
			Stat* followerStats = new Stat(0);

			node_t* node = list_AddNodeLast(followerList);
			node->element = followerStats;
			//node->deconstructor = &followerStats->~Stat;
			node->size = sizeof(followerStats);

			// read follower attributes
			fread(&followerStats->type, sizeof(Monster), 1, fp);
			fread(&followerStats->sex, sizeof(sex_t), 1, fp);
			fread(&followerStats->appearance, sizeof(Uint32), 1, fp);
			fread(&followerStats->name, sizeof(char), 32, fp);
			fread(&followerStats->HP, sizeof(Sint32), 1, fp);
			fread(&followerStats->MAXHP, sizeof(Sint32), 1, fp);
			fread(&followerStats->MP, sizeof(Sint32), 1, fp);
			fread(&followerStats->MAXMP, sizeof(Sint32), 1, fp);
			fread(&followerStats->STR, sizeof(Sint32), 1, fp);
			fread(&followerStats->DEX, sizeof(Sint32), 1, fp);
			fread(&followerStats->CON, sizeof(Sint32), 1, fp);
			fread(&followerStats->INT, sizeof(Sint32), 1, fp);
			fread(&followerStats->PER, sizeof(Sint32), 1, fp);
			fread(&followerStats->CHR, sizeof(Sint32), 1, fp);
			fread(&followerStats->EXP, sizeof(Sint32), 1, fp);
			fread(&followerStats->LVL, sizeof(Sint32), 1, fp);
			fread(&followerStats->GOLD, sizeof(Sint32), 1, fp);
			fread(&followerStats->HUNGER, sizeof(Sint32), 1, fp);

			int j;
			for ( j = 0; j < NUMPROFICIENCIES; j++ )
			{
				if ( versionNumber < 323 && j >= PRO_UNARMED )
				{
					followerStats->PROFICIENCIES[j] = 0;
				}
				else
				{
					fread(&followerStats->PROFICIENCIES[j], sizeof(Sint32), 1, fp);
				}
			}
			for ( j = 0; j < NUMEFFECTS; j++ )
			{
				if ( versionNumber <= 323 ) // legacy
				{
					if ( c < 32 )
					{
						fread(&followerStats->EFFECTS[j], sizeof(bool), 1, fp);
						fread(&followerStats->EFFECTS_TIMERS[j], sizeof(Sint32), 1, fp);
					}
					else
					{
						followerStats->EFFECTS[j] = false;
						followerStats->EFFECTS_TIMERS[j] = 0;
					}
				}
				else
				{
					fread(&followerStats->EFFECTS[j], sizeof(bool), 1, fp);
					fread(&followerStats->EFFECTS_TIMERS[j], sizeof(Sint32), 1, fp);
				}
			}
			if ( versionNumber >= 323 )
			{
				for ( j = 0; j < 32; ++j )
				{
					fread(&followerStats->MISC_FLAGS[j], sizeof(Sint32), 1, fp);
				}
			}

			/*printlog("\n\n ** FOLLOWER #%d **\n", i + 1);
			printlog("Follower stats: \n");
			followerStats->printStats();
			printlog("\n\n");*/

			// item variables
			ItemType type;
			Status status;
			Sint16 beatitude;
			Sint16 count;
			Uint32 appearance;
			bool identified;

			// read follower inventory
			Uint32 invSize = 0;
			fread(&invSize, sizeof(Uint32), 1, fp);
			for ( j = 0; j < invSize; j++ )
			{
				fread(&type, sizeof(ItemType), 1, fp);
				fread(&status, sizeof(Status), 1, fp);
				fread(&beatitude, sizeof(Sint16), 1, fp);
				fread(&count, sizeof(Sint16), 1, fp);
				fread(&appearance, sizeof(Uint32), 1, fp);
				fread(&identified, sizeof(bool), 1, fp);

				Item* item = newItem(type, status, beatitude, count, appearance, identified, &followerStats->inventory);
				fread(&item->x, sizeof(Sint32), 1, fp);
				fread(&item->y, sizeof(Sint32), 1, fp);
			}

			// read follower equipment
			int b;
			for ( b = 0; b < 10; b++ )
			{
				fread(&type, sizeof(ItemType), 1, fp);
				if ( (int)type < NUMITEMS )
				{
					fread(&status, sizeof(Status), 1, fp);
					fread(&beatitude, sizeof(Sint16), 1, fp);
					fread(&count, sizeof(Sint16), 1, fp);
					fread(&appearance, sizeof(Uint32), 1, fp);
					fread(&identified, sizeof(bool), 1, fp);

					Item* item = newItem(type, status, beatitude, count, appearance, identified, NULL);

					switch ( b )
					{
						case 0:
							followerStats->helmet = item;
							break;
						case 1:
							followerStats->breastplate = item;
							break;
						case 2:
							followerStats->gloves = item;
							break;
						case 3:
							followerStats->shoes = item;
							break;
						case 4:
							followerStats->shield = item;
							break;
						case 5:
							followerStats->weapon = item;
							break;
						case 6:
							followerStats->cloak = item;
							break;
						case 7:
							followerStats->amulet = item;
							break;
						case 8:
							followerStats->ring = item;
							break;
						case 9:
							followerStats->mask = item;
							break;
					}
				}
			}
		}
	}

	fclose(fp);
	return followers;
}

/*-------------------------------------------------------------------------------

	deleteSaveGame

	Deletes the saved game

-------------------------------------------------------------------------------*/

int deleteSaveGame(int gametype, int saveIndex)
{
	char savefile[PATH_MAX] = "";
	char path[PATH_MAX] = "";
	if ( gametype == SINGLE )
	{
		strncpy(savefile, setSaveGameFileName(true, false, saveIndex).c_str(), PATH_MAX - 1);
	}
	else
	{
		strncpy(savefile, setSaveGameFileName(false, false, saveIndex).c_str(), PATH_MAX - 1);
	}
	completePath(path, savefile, outputdir);

	if (access(path, F_OK) != -1)
	{
		printlog("deleting savegame in '%s'...\n", path);
		int result = remove(path);
		if (result)
		{
			printlog("warning: failed to delete savegame in '%s'!\n", path);
#ifdef _MSC_VER
			printlog(strerror(errno));
#endif
		}
	}

	if ( gametype == SINGLE )
	{
		strncpy(savefile, setSaveGameFileName(true, true, saveIndex).c_str(), 63);
	}
	else
	{
		strncpy(savefile, setSaveGameFileName(false, true, saveIndex).c_str(), 63);
	}
	completePath(path, savefile, outputdir);

	if (access(path, F_OK) != -1)
	{
		printlog("deleting savegame in '%s'...\n", path);
		int result = remove(path);
		if (result)
		{
			printlog("warning: failed to delete savegame in '%s'!\n", path);
#ifdef _MSC_VER
			printlog(strerror(errno));
#endif
		}
		return result;
	}
	else
	{
		return 0;
	}
}

/*-------------------------------------------------------------------------------

	saveGameExists

	checks to see if a valid save game exists.

-------------------------------------------------------------------------------*/

bool saveGameExists(bool singleplayer, int saveIndex)
{
	char savefile[PATH_MAX] = "";
	char path[PATH_MAX] = "";
	strncpy(savefile, setSaveGameFileName(singleplayer, false, saveIndex).c_str(), PATH_MAX - 1);
	completePath(path, savefile, outputdir);

	if ( access(path, F_OK ) == -1 )
	{
		return false;
	}
	else
	{
		FILE* fp;
		if ( (fp = fopen(path, "rb")) == NULL )
		{
			return false;
		}
		char checkstr[64];
		fread(checkstr, sizeof(char), strlen("BARONYSAVEGAME"), fp);
		if ( strncmp(checkstr, "BARONYSAVEGAME", strlen("BARONYSAVEGAME")) )
		{
			fclose(fp);
			return false;
		}
		fread(checkstr, sizeof(char), strlen(VERSION), fp);
		int versionNumber = getSavegameVersion(checkstr);
		if ( versionNumber == -1 )
		{
			// if getSavegameVersion returned -1, abort.
			fclose(fp);
			return false;
		}
		fclose(fp);
		return true;
	}
}

/*-------------------------------------------------------------------------------

	getSaveGameName

	Gets the name of the character in the saved game

-------------------------------------------------------------------------------*/

char* getSaveGameName(bool singleplayer, int saveIndex)
{
	char name[128];
	FILE* fp;
	int c;

	int level, class_;
	int mul, plnum, dungeonlevel;
	int playerRace, playerAppearance;

	char* tempstr = (char*) calloc(1024, sizeof(char));
	char savefile[PATH_MAX] = "";
	char path[PATH_MAX] = "";
	strncpy(savefile, setSaveGameFileName(singleplayer, false, saveIndex).c_str(), PATH_MAX - 1);
	completePath(path, savefile, outputdir);

	// open file
	if ( (fp = fopen(path, "rb")) == NULL )
	{
		printlog("error: failed to check name in '%s'!\n", path);
		free(tempstr);
		return NULL;
	}

	// read from file
	char checkstr[64];
	fread(checkstr, sizeof(char), strlen("BARONYSAVEGAME"), fp);
	if ( strncmp(checkstr, "BARONYSAVEGAME", strlen("BARONYSAVEGAME")) )
	{
		printlog("error: '%s' is corrupt!\n", path);
		fclose(fp);
		free(tempstr);
		return NULL;
	}
	fread(checkstr, sizeof(char), strlen(VERSION), fp);
	int versionNumber = getSavegameVersion(checkstr);
	printlog("getSaveGameName: '%s' version number %d", savefile, versionNumber);
	if ( versionNumber == -1 )
	{
		// if getSavegameVersion returned -1, abort.
		printlog("error: '%s' is corrupt!\n", path);
		fclose(fp);
		free(tempstr);
		return nullptr;
	}

	fseek(fp, sizeof(Uint32), SEEK_CUR);
	fread(&mul, sizeof(Uint32), 1, fp);
	fread(&plnum, sizeof(Uint32), 1, fp);
	fseek(fp, sizeof(Uint32), SEEK_CUR);
	fread(&dungeonlevel, sizeof(Uint32), 1, fp);
	dungeonlevel = dungeonlevel & 0xFF;
	fseek(fp,  sizeof(bool), SEEK_CUR);
	if ( versionNumber >= 310 )
	{
		fseek(fp, sizeof(Sint32) * NUM_CONDUCT_CHALLENGES, SEEK_CUR);
		fseek(fp, sizeof(Sint32) * NUM_GAMEPLAY_STATISTICS, SEEK_CUR);
	}
	if ( versionNumber >= 335 )
	{
		fseek(fp, sizeof(Uint32), SEEK_CUR); // svFlags
	}
	fseek(fp, sizeof(Uint32)*NUM_HOTBAR_SLOTS, SEEK_CUR);
	fseek(fp, sizeof(Uint32) + sizeof(bool) + sizeof(bool) + sizeof(bool) + sizeof(bool), SEEK_CUR);

	int numspells = 0;
	fread(&numspells, sizeof(Uint32), 1, fp);
	for ( c = 0; c < numspells; c++ )
	{
		fseek(fp, sizeof(Uint32), SEEK_CUR);
	}

	int monsters = NUMMONSTERS;
	if ( versionNumber < 325 )
	{
		monsters = 33;
	}

	// skip through other player data until you get to the correct player
	for ( c = 0; c < plnum; c++ )
	{
		fseek(fp, sizeof(Uint32), SEEK_CUR);
		fseek(fp, monsters * sizeof(Sint32), SEEK_CUR);
		fseek(fp, sizeof(Monster), SEEK_CUR);
		fseek(fp, sizeof(sex_t), SEEK_CUR);
		fseek(fp, sizeof(Uint32), SEEK_CUR);
		fseek(fp, sizeof(char) * 32, SEEK_CUR);
		fseek(fp, sizeof(Sint32), SEEK_CUR);
		fseek(fp, sizeof(Sint32), SEEK_CUR);
		fseek(fp, sizeof(Sint32), SEEK_CUR);
		fseek(fp, sizeof(Sint32), SEEK_CUR);
		fseek(fp, sizeof(Sint32), SEEK_CUR);
		fseek(fp, sizeof(Sint32), SEEK_CUR);
		fseek(fp, sizeof(Sint32), SEEK_CUR);
		fseek(fp, sizeof(Sint32), SEEK_CUR);
		fseek(fp, sizeof(Sint32), SEEK_CUR);
		fseek(fp, sizeof(Sint32), SEEK_CUR);
		fseek(fp, sizeof(Sint32), SEEK_CUR);
		fseek(fp, sizeof(Sint32), SEEK_CUR);
		fseek(fp, sizeof(Sint32), SEEK_CUR);
		fseek(fp, sizeof(Sint32), SEEK_CUR);
		if ( versionNumber >= 323 )
		{
			fseek(fp, sizeof(Sint32)*NUMPROFICIENCIES, SEEK_CUR);
		}
		else
		{
			fseek(fp, sizeof(Sint32)*14, SEEK_CUR);
		}
		if ( versionNumber <= 323 )
		{
			fseek(fp, sizeof(bool)*32, SEEK_CUR);
			fseek(fp, sizeof(Sint32)*32, SEEK_CUR);
		}
		else
		{
			fseek(fp, sizeof(bool)*NUMEFFECTS, SEEK_CUR);
			fseek(fp, sizeof(Sint32)*NUMEFFECTS, SEEK_CUR);
		}
		if ( versionNumber >= 323 )
		{
			fseek(fp, sizeof(Sint32) * 32, SEEK_CUR); // stat flags
		}

		if ( plnum == 0 )
		{
			// server needs to skip past its inventory
			int numitems = 0;
			fread(&numitems, sizeof(Uint32), 1, fp);

			int i;
			for ( i = 0; i < numitems; i++ )
			{
				fseek(fp, sizeof(ItemType), SEEK_CUR);
				fseek(fp, sizeof(Status), SEEK_CUR);
				fseek(fp, sizeof(Sint16), SEEK_CUR);
				fseek(fp, sizeof(Sint16), SEEK_CUR);
				fseek(fp, sizeof(Uint32), SEEK_CUR);
				fseek(fp, sizeof(bool), SEEK_CUR);
				fseek(fp, sizeof(Sint32), SEEK_CUR);
				fseek(fp, sizeof(Sint32), SEEK_CUR);
			}
			fseek(fp, sizeof(Uint32) * 10, SEEK_CUR); // equipment slots
		}
		else
		{
			// client needs to skip the dummy byte
			fseek(fp, sizeof(Status), SEEK_CUR);
		}
	}

	fread(&class_, sizeof(Uint32), 1, fp);
	for ( c = 0; c < monsters; c++ )
	{
		fseek(fp, sizeof(Sint32), SEEK_CUR);
	}
	fseek(fp, sizeof(Monster) + sizeof(sex_t), SEEK_CUR);
	Uint32 raceAndAppearance = 0;
	fread(&raceAndAppearance, sizeof(Uint32), 1, fp);
	playerAppearance = raceAndAppearance & 0xFF;
	playerRace = (raceAndAppearance & 0xFF00) >> 8;
	fread(&name, sizeof(char), 32, fp);
	name[32] = 0;
	fseek(fp, sizeof(Sint32) * 11, SEEK_CUR);
	fread(&level, sizeof(Sint32), 1, fp);

	// assemble string
	char timestamp[128] = "";
#ifdef WINDOWS
	struct _stat result;
	if ( _stat(path, &result) == 0 )
	{
		struct tm *tm = localtime(&result.st_mtime);
		if ( tm )
		{
			errno_t err = strftime(timestamp, 127, "%d %b %Y, %H:%M", tm); //day, month, year, time
		}
	}
#else
	struct stat result;
	if ( stat(path, &result) == 0 )
	{
		struct tm *tm = localtime(&result.st_mtime);
		if ( tm )
		{
			strftime(timestamp, 127, "%d %b %Y, %H:%M", tm); //day, month, year, time
		}
	}
#endif // WINDOWS

	int plnumTemp = plnum;
	if ( plnumTemp >= MAXPLAYERS )
	{
		plnumTemp = MAXPLAYERS - 1; // fix for loading 16-player savefile in normal Barony. plnum might be out of index for stats[]
	}
	int oldRace = stats[plnumTemp]->playerRace;
	stats[plnumTemp]->playerRace = playerRace;

	if ( mul == DIRECTCLIENT || mul == CLIENT )
	{
		// include the player number in the printf.
		snprintf(tempstr, 1024, language[1540 + mul], name, level, playerClassLangEntry(class_, plnumTemp), dungeonlevel, plnum, timestamp);
	}
	else
	{
		if ( mul == SERVERCROSSPLAY )
		{
			mul = SERVER;
		}
		snprintf(tempstr, 1024, language[1540 + mul], name, level, playerClassLangEntry(class_, plnumTemp), dungeonlevel, timestamp);
	}
	// close file
	stats[plnumTemp]->playerRace = oldRace;
	fclose(fp);

	return tempstr;
}

/*-------------------------------------------------------------------------------

	getSaveGameUniqueGameKey

	Returns the uniqueGameKey variable stored in the save game

-------------------------------------------------------------------------------*/

Uint32 getSaveGameUniqueGameKey(bool singleplayer, int saveIndex)
{
	FILE* fp;
	Uint32 gameKey;
	char savefile[PATH_MAX] = "";
	char path[PATH_MAX] = "";
	strncpy(savefile, setSaveGameFileName(singleplayer, false, saveIndex).c_str(), PATH_MAX - 1);
	completePath(path, savefile, outputdir);

	// open file
	if ( (fp = fopen(path, "rb")) == NULL )
	{
		printlog("error: failed to get map seed out of '%s'!\n", path);
		return 0;
	}

	// read from file
	char checkstr[64];
	fread(checkstr, sizeof(char), strlen("BARONYSAVEGAME"), fp);
	if ( strncmp(checkstr, "BARONYSAVEGAME", strlen("BARONYSAVEGAME")) )
	{
		printlog("error: '%s' is corrupt!\n", path);
		fclose(fp);
		return 0;
	}
	fread(checkstr, sizeof(char), strlen(VERSION), fp);
	int versionNumber = getSavegameVersion(checkstr);
	if ( versionNumber == -1 )
	{
		// if getSavegameVersion returned -1, abort.
		printlog("error: '%s' is corrupt!\n", path);
		fclose(fp);
		return 0;
	}

	fread(&gameKey, sizeof(Uint32), 1, fp);

	// close file
	fclose(fp);
	return gameKey;
}

/*-------------------------------------------------------------------------------

getSaveGameVersionNum

Returns the savefile version

-------------------------------------------------------------------------------*/

int getSaveGameVersionNum(bool singleplayer, int saveIndex)
{
	FILE* fp;
	Uint32 gameKey;
	char savefile[PATH_MAX] = "";
	char path[PATH_MAX] = "";
	strncpy(savefile, setSaveGameFileName(singleplayer, false, saveIndex).c_str(), PATH_MAX - 1);
	completePath(path, savefile, outputdir);

	// open file
	if ( (fp = fopen(path, "rb")) == NULL )
	{
		printlog("error: failed to get map seed out of '%s'!\n", path);
		return 0;
	}

	// read from file
	char checkstr[64];
	fread(checkstr, sizeof(char), strlen("BARONYSAVEGAME"), fp);
	if ( strncmp(checkstr, "BARONYSAVEGAME", strlen("BARONYSAVEGAME")) )
	{
		printlog("error: '%s' is corrupt!\n", path);
		fclose(fp);
		return 0;
	}
	fread(checkstr, sizeof(char), strlen(VERSION), fp);
	int versionNumber = getSavegameVersion(checkstr);
	if ( versionNumber == -1 )
	{
		printlog("error: '%s' is corrupt!\n", path);
	}

	// close file
	fclose(fp);
	return versionNumber;
}

/*-------------------------------------------------------------------------------

	getSaveGameType

	Returns the multiplayer variable stored in the save game

-------------------------------------------------------------------------------*/

int getSaveGameType(bool singleplayer, int saveIndex)
{
	FILE* fp;
	int mul;
	char savefile[PATH_MAX] = "";
	char path[PATH_MAX] = "";
	strncpy(savefile, setSaveGameFileName(singleplayer, false, saveIndex).c_str(), PATH_MAX - 1);
	completePath(path, savefile, outputdir);

	// open file
	if ( (fp = fopen(path, "rb")) == NULL )
	{
		printlog("error: failed to get game type out of '%s'!\n", path);
		return 0;
	}

	// read from file
	char checkstr[64];
	fread(checkstr, sizeof(char), strlen("BARONYSAVEGAME"), fp);
	if ( strncmp(checkstr, "BARONYSAVEGAME", strlen("BARONYSAVEGAME")) )
	{
		printlog("error: '%s' is corrupt!\n", path);
		fclose(fp);
		return 0;
	}
	fread(checkstr, sizeof(char), strlen(VERSION), fp);
	int versionNumber = getSavegameVersion(checkstr);
	if ( versionNumber == -1 )
	{
		// if getSavegameVersion returned -1, abort.
		printlog("error: '%s' is corrupt!\n", path);
		fclose(fp);
		return 0;
	}

	fseek(fp, sizeof(Uint32), SEEK_CUR);
	fread(&mul, sizeof(Uint32), 1, fp);

	// close file
	fclose(fp);
	return mul;
}

/*-------------------------------------------------------------------------------

	getSaveGameClientnum

	Returns the clientnum variable stored in the save game

-------------------------------------------------------------------------------*/

int getSaveGameClientnum(bool singleplayer, int saveIndex)
{
	FILE* fp;
	int clientnum;
	char savefile[PATH_MAX] = "";
	char path[PATH_MAX] = "";
	strncpy(savefile, setSaveGameFileName(singleplayer, false, saveIndex).c_str(), PATH_MAX - 1);
	completePath(path, savefile, outputdir);

	// open file
	if ( (fp = fopen(path, "rb")) == NULL )
	{
		printlog("error: failed to get clientnum out of '%s'!\n", path);
		return 0;
	}

	// read from file
	char checkstr[64];
	fread(checkstr, sizeof(char), strlen("BARONYSAVEGAME"), fp);
	if ( strncmp(checkstr, "BARONYSAVEGAME", strlen("BARONYSAVEGAME")) )
	{
		printlog("error: '%s' is corrupt!\n", path);
		fclose(fp);
		return 0;
	}
	fread(checkstr, sizeof(char), strlen(VERSION), fp);
	int versionNumber = getSavegameVersion(checkstr);
	if ( versionNumber == -1 )
	{
		// if getSavegameVersion returned -1, abort.
		printlog("error: '%s' is corrupt!\n", path);
		fclose(fp);
		return 0;
	}

	fseek(fp, sizeof(Uint32), SEEK_CUR);
	fseek(fp, sizeof(Uint32), SEEK_CUR);
	fread(&clientnum, sizeof(Uint32), 1, fp);

	// close file
	fclose(fp);
	return clientnum;
}

/*-------------------------------------------------------------------------------

	getSaveGameMapSeed

	Returns the mapseed variable stored in the save game

-------------------------------------------------------------------------------*/

Uint32 getSaveGameMapSeed(bool singleplayer, int saveIndex)
{
	FILE* fp;
	Uint32 seed;
	char savefile[PATH_MAX] = "";
	char path[PATH_MAX] = "";
	strncpy(savefile, setSaveGameFileName(singleplayer, false, saveIndex).c_str(), PATH_MAX - 1);
	completePath(path, savefile, outputdir);

	// open file
	if ( (fp = fopen(path, "rb")) == NULL )
	{
		printlog("error: failed to get map seed out of '%s'!\n", path);
		return 0;
	}

	// read from file
	char checkstr[64];
	fread(checkstr, sizeof(char), strlen("BARONYSAVEGAME"), fp);
	if ( strncmp(checkstr, "BARONYSAVEGAME", strlen("BARONYSAVEGAME")) )
	{
		printlog("error: '%s' is corrupt!\n", path);
		fclose(fp);
		return 0;
	}
	fread(checkstr, sizeof(char), strlen(VERSION), fp);
	int versionNumber = getSavegameVersion(checkstr);
	if ( versionNumber == -1 )
	{
		// if getSavegameVersion returned -1, abort.
		printlog("error: '%s' is corrupt!\n", path);
		fclose(fp);
		return 0;
	}

	fseek(fp, sizeof(Uint32), SEEK_CUR);
	fseek(fp, sizeof(Uint32), SEEK_CUR);
	fseek(fp, sizeof(Uint32), SEEK_CUR);
	fread(&seed, sizeof(Uint32), 1, fp);

	// close file
	fclose(fp);
	return seed;
}

int getSavegameVersion(char checkstr[64])
{
	int versionNumber = 300;
	char versionStr[4] = "000";
	int i = 0;
	for ( int j = 0; j < strlen(VERSION); ++j )
	{
		if ( checkstr[j] >= '0' && checkstr[j] <= '9' )
		{
			versionStr[i] = checkstr[j]; // copy all integers into versionStr.
			++i;
			if ( i == 3 )
			{
				versionStr[i] = '\0';
				break; // written 3 characters, add termination and break loop.
			}
		}
	}
	versionNumber = atoi(versionStr); // convert from string to int.
	if ( versionNumber < 200 || versionNumber > 999 )
	{
		// if version number less than v2.0.0, or more than 3 digits, abort.
		return -1;
	}
	return versionNumber;
}

void setDefaultPlayerConducts()
{
	conductPenniless = true;
	conductFoodless = true;
	conductVegetarian = true;
	conductIlliterate = true;

	for ( int c = 0; c < NUM_CONDUCT_CHALLENGES; ++c )
	{
		conductGameChallenges[c] = 0;
	}
	conductGameChallenges[CONDUCT_HARDCORE] = 1;
	conductGameChallenges[CONDUCT_CHEATS_ENABLED] = 0;
	conductGameChallenges[CONDUCT_CLASSIC_MODE] = 0;
	conductGameChallenges[CONDUCT_BRAWLER] = 1;
	conductGameChallenges[CONDUCT_RANGED_ONLY] = 1;
	conductGameChallenges[CONDUCT_MODDED] = 0;
	conductGameChallenges[CONDUCT_LIFESAVING] = 0;
	conductGameChallenges[CONDUCT_KEEPINVENTORY] = 0;

	for ( int c = 0; c < NUM_GAMEPLAY_STATISTICS; ++c )
	{
		gameStatistics[c] = 0;
	}
	for ( int c = 0; c < MAXPLAYERS; ++c )
	{
		achievementStatusRhythmOfTheKnight[c] = false;
		achievementStatusStrobe[c] = false;
		achievementStatusThankTheTank[c] = false;
		achievementRhythmOfTheKnightVec[c].clear();
		achievementThankTheTankPair[c].first = 0;
		achievementThankTheTankPair[c].second = 0;
		achievementStrobeVec[c].clear();
		achievementStatusBaitAndSwitch[c] = false;
		achievementBaitAndSwitchTimer[c] = 0;
		playerFailedRangedOnlyConduct[c] = false;
	}
	clientLearnedAlchemyIngredients.clear();
	achievementObserver.clearPlayerAchievementData();
}

void updatePlayerConductsInMainLoop()
{
	if ( conductPenniless )
	{
		if ( stats[clientnum]->GOLD > 0 )
		{
			conductPenniless = false;
		}
	}
	if ( !conductGameChallenges[CONDUCT_KEEPINVENTORY] )
	{
		if ( (svFlags & SV_FLAG_KEEPINVENTORY) )
		{
			if ( multiplayer != SINGLE )
			{
				conductGameChallenges[CONDUCT_KEEPINVENTORY] = 1;
			}
		}
	}
	if ( !conductGameChallenges[CONDUCT_LIFESAVING] )
	{
		if ( (svFlags & SV_FLAG_LIFESAVING) )
		{
			conductGameChallenges[CONDUCT_LIFESAVING] = 1;
		}
	}
	if ( conductGameChallenges[CONDUCT_HARDCORE] )
	{
		if ( !(svFlags & SV_FLAG_HARDCORE) )
		{
			conductGameChallenges[CONDUCT_HARDCORE] = 0;
		}
	}
	if ( !conductGameChallenges[CONDUCT_CHEATS_ENABLED] )
	{
		if ( (svFlags & SV_FLAG_CHEATS) )
		{
			conductGameChallenges[CONDUCT_CHEATS_ENABLED] = 1;
		}
	}
	if ( !conductGameChallenges[CONDUCT_MULTIPLAYER] )
	{
		if ( multiplayer != SINGLE )
		{
			conductGameChallenges[CONDUCT_MULTIPLAYER] = 1;
		}
	}
	if ( !conductGameChallenges[CONDUCT_CLASSIC_MODE] )
	{
		if ( (svFlags & SV_FLAG_CLASSIC) )
		{
			conductGameChallenges[CONDUCT_CLASSIC_MODE] = 1;
		}
	}
	if ( !conductGameChallenges[CONDUCT_MODDED] )
	{
		if ( gamemods_numCurrentModsLoaded > 0 )
		{
			conductGameChallenges[CONDUCT_MODDED] = 1;
		}
	}

	achievementObserver.achievementTimersTickDown();
}

void updateGameplayStatisticsInMainLoop()
{
	// local player only here.
	if ( gameStatistics[STATISTICS_BOMB_SQUAD] >= 5 )
	{
		steamAchievement("BARONY_ACH_BOMB_SQUAD");
	}
	if ( gameStatistics[STATISTICS_SITTING_DUCK] >= 10 )
	{
		steamAchievement("BARONY_ACH_SITTING_DUCK");
	}
	if ( gameStatistics[STATISTICS_YES_WE_CAN] >= 10 )
	{
		steamAchievement("BARONY_ACH_YES_WE_CAN");
	}
	if ( gameStatistics[STATISTICS_FIRE_MAYBE_DIFFERENT] >= 2 )
	{
		steamAchievement("BARONY_ACH_FIRE_MAYBE_DIFFERENT");
	}
	if ( gameStatistics[STATISTICS_HEAL_BOT] >= 1000 )
	{
		steamAchievement("BARONY_ACH_HEAL_BOT");
	}
	if ( gameStatistics[STATISTICS_HOT_TUB_TIME_MACHINE] >= 50 )
	{
		steamAchievement("BARONY_ACH_HOT_TUB");
	}
	if ( gameStatistics[STATISTICS_FUNCTIONAL] >= 10 )
	{
		steamAchievement("BARONY_ACH_FUNCTIONAL");
	}
	if ( gameStatistics[STATISTICS_OHAI_MARK] >= 20 )
	{
		steamAchievement("BARONY_ACH_OHAI_MARK");
	}
	if ( gameStatistics[STATISTICS_PIMPING_AINT_EASY] >= 6 )
	{
		steamAchievement("BARONY_ACH_PIMPIN");
	}
	if ( gameStatistics[STATISTICS_TRIBE_SUBSCRIBE] >= 4 )
	{
		steamAchievement("BARONY_ACH_TRIBE_SUBSCRIBE");
	}
	if ( gameStatistics[STATISTICS_FORUM_TROLL] > 0 )
	{
		int walls = gameStatistics[STATISTICS_FORUM_TROLL] & 0xFF;
		int trolls = ((gameStatistics[STATISTICS_FORUM_TROLL] >> 8) & 0xFF);
		int fears = ((gameStatistics[STATISTICS_FORUM_TROLL] >> 16) & 0xFF);
		if ( walls == 3 && trolls == 3 && fears == 3 )
		{
			steamAchievement("BARONY_ACH_FORUM_TROLL");
		}
	}

	if ( gameStatistics[STATISTICS_TEMPT_FATE] == -1 )
	{
		steamAchievement("BARONY_ACH_TEMPT_FATE");
	}
	else if ( gameStatistics[STATISTICS_TEMPT_FATE] > 0 )
	{
		// tick down 5 sec counter for achievement, this function called once per second.
		--gameStatistics[STATISTICS_TEMPT_FATE];
		if ( gameStatistics[STATISTICS_TEMPT_FATE] < 0 )
		{
			gameStatistics[STATISTICS_TEMPT_FATE] = 0;
		}
	}

	if ( gameStatistics[STATISTICS_ALCHEMY_RECIPES] != 0 && clientLearnedAlchemyIngredients.empty() )
	{
		int numpotions = static_cast<int>(potionStandardAppearanceMap.size());
		for ( int i = 0; i < numpotions; ++i )
		{
			bool learned = gameStatistics[STATISTICS_ALCHEMY_RECIPES] & (1 << i);
			if ( learned )
			{
				auto typeAppearance = potionStandardAppearanceMap.at(i);
				int type = typeAppearance.first;
				clientLearnedAlchemyIngredients.insert(type);
			}
		}
	}

	if ( (ticks % (TICKS_PER_SECOND * 8) == 0) && gameStatistics[STATISTICS_ALCHEMY_RECIPES] != 0 )
	{
		int numpotions = static_cast<int>(potionStandardAppearanceMap.size());
		bool failAchievement = false;
		for ( int i = 0; i < numpotions; ++i )
		{
			bool learned = gameStatistics[STATISTICS_ALCHEMY_RECIPES] & (1 << i);
			auto typeAppearance = potionStandardAppearanceMap.at(i);
			int type = typeAppearance.first;
			if ( !learned && (GenericGUI.isItemBaseIngredient(type) || GenericGUI.isItemSecondaryIngredient(type)) )
			{
				failAchievement = true;
				break;
			}
		}
		if ( !failAchievement )
		{
			steamAchievement("BARONY_ACH_MIXOLOGIST");
		}
	}

	if ( (ticks % (TICKS_PER_SECOND * 8) == 0) && (gameStatistics[STATISTICS_POP_QUIZ_1] != 0 || gameStatistics[STATISTICS_POP_QUIZ_2] != 0) )
	{
		int numSpellsCast = 0;
		int stat1 = gameStatistics[STATISTICS_POP_QUIZ_1];
		int stat2 = gameStatistics[STATISTICS_POP_QUIZ_1];
		for ( int i = 0; i < 30; ++i )
		{
			// count the bits set.
			numSpellsCast += (stat1 & 1);
			numSpellsCast += (stat2 & 1);
			stat1 = stat1 >> 1;
			stat2 = stat2 >> 1;
		}
		if ( numSpellsCast >= 20 )
		{
			steamAchievement("BARONY_ACH_POP_QUIZ");
		}
	}

	if ( ticks % (TICKS_PER_SECOND * 10) == 0 )
	{
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			if ( (i == clientnum) || (multiplayer == SERVER && i != clientnum) )
			{
				// clients update their own total, server can update clients.
				if ( achievementObserver.playerAchievements[i].torchererScrap > 0 )
				{
					if ( i == clientnum )
					{
						steamStatisticUpdate(STEAM_STAT_TORCHERER, STEAM_STAT_INT, achievementObserver.playerAchievements[i].torchererScrap);
					}
					else
					{
						steamStatisticUpdateClient(i, STEAM_STAT_TORCHERER, STEAM_STAT_INT, 
							achievementObserver.playerAchievements[i].torchererScrap);
					}
					achievementObserver.playerAchievements[i].torchererScrap = 0;
				}
				if ( achievementObserver.playerAchievements[i].superShredder > 0 )
				{
					if ( i == clientnum )
					{
						steamStatisticUpdate(STEAM_STAT_SUPER_SHREDDER, STEAM_STAT_INT, achievementObserver.playerAchievements[i].superShredder);
					}
					else
					{
						steamStatisticUpdateClient(i, STEAM_STAT_SUPER_SHREDDER, STEAM_STAT_INT, achievementObserver.playerAchievements[i].superShredder);
					}
					achievementObserver.playerAchievements[i].superShredder = 0;
				}
				if ( achievementObserver.playerAchievements[i].fixerUpper > 0 )
				{
					if ( i == clientnum )
					{
						steamStatisticUpdate(STEAM_STAT_FIXER_UPPER, STEAM_STAT_INT, achievementObserver.playerAchievements[i].fixerUpper);
					}
					else
					{
						steamStatisticUpdateClient(i, STEAM_STAT_FIXER_UPPER, STEAM_STAT_INT, achievementObserver.playerAchievements[i].fixerUpper);
					}
					achievementObserver.playerAchievements[i].fixerUpper = 0;
				}
			}
		}
	}

	if ( ticks % (TICKS_PER_SECOND * 5) == 0 )
	{
		std::unordered_set<int> potionList;
		std::unordered_set<int> ammoList;
		std::unordered_set<int> bowList;
		std::unordered_set<int> utilityBeltList;
		int badAndBeautiful = -1;
		if ( stats[clientnum]->appearance == 0 && (stats[clientnum]->type == INCUBUS || stats[clientnum]->type == SUCCUBUS) )
		{
			if ( stats[clientnum]->playerRace == RACE_INCUBUS || stats[clientnum]->playerRace == RACE_SUCCUBUS )
			{
				badAndBeautiful = 0;
			}
		}
		int dummy1 = 0;
		int dummy2 = 0;
		for ( node_t* node = stats[clientnum]->inventory.first; node != nullptr; node = node->next )
		{
			Item* item = (Item*)node->element;
			if ( item )
			{
				if ( itemCategory(item) == POTION )
				{
					switch ( item->type )
					{
						case POTION_EMPTY:
						case POTION_THUNDERSTORM:
						case POTION_ICESTORM:
						case POTION_STRENGTH:
						case POTION_FIRESTORM:
							// do nothing, these are brewed only potions
							break;
						default:
							potionList.insert(item->type);
							break;
					}
				}
				else if ( client_classes[clientnum] == CLASS_HUNTER && itemTypeIsQuiver(item->type) )
				{
					ammoList.insert(item->type);
				}
				else if ( client_classes[clientnum] == CLASS_HUNTER && isRangedWeapon(*item) )
				{
					if ( item->type == CROSSBOW || item->type == HEAVY_CROSSBOW )
					{
						bowList.insert(CROSSBOW);
					}
					else if ( item->type == SHORTBOW || item->type == LONGBOW || item->type == COMPOUND_BOW )
					{
						bowList.insert(SHORTBOW);
					}
				}
				if ( GenericGUI.tinkeringGetCraftingCost(item, &dummy1, &dummy2) )
				{
					utilityBeltList.insert(item->type);
				}
				if ( badAndBeautiful >= 0 && item->identified && item->beatitude < 0 && itemIsEquipped(item, clientnum) )
				{
					++badAndBeautiful;
				}
			}
		}
		if ( potionList.size() >= 16 )
		{
			steamAchievement("BARONY_ACH_POTION_PREPARATION");
		}
		if ( ammoList.size() >= 7 && bowList.size() >= 2 )
		{
			steamAchievement("BARONY_ACH_ARSENAL");
		}
		if ( utilityBeltList.size() >= 16 )
		{
			steamAchievement("BARONY_ACH_UTILITY_BELT");
		}
		if ( badAndBeautiful >= 4 )
		{
			steamAchievement("BARONY_ACH_BAD_BEAUTIFUL");
		}

		if ( multiplayer != CLIENT )
		{
			for ( int i = 0; i < MAXPLAYERS; ++i )
			{
				// server only will have these numbers here.
				if ( achievementObserver.playerAchievements[i].ifYouLoveSomething > 0 )
				{
					steamStatisticUpdateClient(i, STEAM_STAT_IF_YOU_LOVE_SOMETHING, STEAM_STAT_INT, achievementObserver.playerAchievements[i].ifYouLoveSomething);
					achievementObserver.playerAchievements[i].ifYouLoveSomething = 0;
				}
				if ( achievementObserver.playerAchievements[i].socialButterfly > 0 )
				{
					steamStatisticUpdateClient(i, STEAM_STAT_SOCIAL_BUTTERFLY, STEAM_STAT_INT, achievementObserver.playerAchievements[i].socialButterfly);
					achievementObserver.playerAchievements[i].socialButterfly = 0;
				}
				if ( achievementObserver.playerAchievements[i].trashCompactor > 0 )
				{
					steamStatisticUpdateClient(i, STEAM_STAT_TRASH_COMPACTOR, STEAM_STAT_INT, achievementObserver.playerAchievements[i].trashCompactor);
					achievementObserver.playerAchievements[i].trashCompactor = 0;
				}
			}
		}
	}
}

std::string setSaveGameFileName(bool singleplayer, bool followersFile, int saveIndex)
{
	std::string filename = "savegames/savegame" + std::to_string(saveIndex);

	//OLD FORMAT
	//#define SAVEGAMEFILE "savegame.dat"
	//#define SAVEGAMEFILE2 "savegame2.dat" // saves follower data
	//#define SAVEGAMEFILE_MULTIPLAYER "savegame_multiplayer.dat"
	//#define SAVEGAMEFILE2_MULTIPLAYER "savegame2_multiplayer.dat" // saves follower data
	//#define SAVEGAMEFILE_MODDED "savegame_modded.dat"
	//#define SAVEGAMEFILE2_MODDED "savegame2_modded.dat"
	//#define SAVEGAMEFILE_MODDED_MULTIPLAYER "savegame_modded_multiplayer.dat"
	//#define SAVEGAMEFILE2_MODDED_MULTIPLAYER "savegame2_modded_multiplayer.dat"

	if ( !followersFile )
	{
		if ( singleplayer )
		{
			if ( gamemods_numCurrentModsLoaded == -1 )
			{
				filename.append(".dat");
			}
			else
			{
				filename.append("_modded.dat");
			}
		}
		else
		{
			if ( gamemods_numCurrentModsLoaded == -1 )
			{
				filename.append("_mp.dat");
			}
			else
			{
				filename.append("_mp_modded.dat");
			}
		}
	}
	else
	{
		if ( singleplayer )
		{
			if ( gamemods_numCurrentModsLoaded == -1 )
			{
				filename.append("_npcs.dat");
			}
			else
			{
				filename.append("_npcs_modded.dat");
			}
		}
		else
		{
			if ( gamemods_numCurrentModsLoaded == -1 )
			{
				filename.append("_mp_npcs.dat");
			}
			else
			{
				filename.append("_mp_npcs_modded.dat");
			}
		}
	}
	return filename;
}

bool anySaveFileExists()
{
	for ( int fileNumber = 0; fileNumber < SAVE_GAMES_MAX; ++fileNumber )
	{
		if ( saveGameExists(true, fileNumber) )
		{
			return true;
		}
	}
	for ( int fileNumber = 0; fileNumber < SAVE_GAMES_MAX; ++fileNumber )
	{
		if ( saveGameExists(false, fileNumber) )
		{
			return true;
		}
	}
	return false;
}

bool findVictory3()
{
	for ( node_t* node = topscores.first; node != nullptr; node = node->next )
	{
		score_t* score = (score_t*)node->element;
		if ( score && score->victory == 3 )
		{
			return true;
		}
	}
	for ( node_t* node = topscoresMultiplayer.first; node != nullptr; node = node->next )
	{
		score_t* score = (score_t*)node->element;
		if ( score && score->victory == 3 )
		{
			return true;
		}
	}

	return false;
}

void updateAchievementRhythmOfTheKnight(int player, Entity* target, bool playerIsHit)
{
	if ( achievementStatusRhythmOfTheKnight[player] || multiplayer == CLIENT
		|| player < 0 || player >= MAXPLAYERS )
	{
		return;
	}

	Uint32 targetUid = target->getUID();

	if ( !playerIsHit )
	{
		// player attacking a monster, needs to be after a block (vec size 1, 3 or 5)
		if ( !achievementRhythmOfTheKnightVec[player].empty() )
		{
			if ( achievementRhythmOfTheKnightVec[player].at(0).second != targetUid ) 
			{
				// check first uid entry, if not matching the monster, we swapped targets and should reset.
				achievementRhythmOfTheKnightVec[player].clear();
				//messagePlayer(0, "cleared, not attacking same target");
				return;
			}
			else
			{
				int size = achievementRhythmOfTheKnightVec[player].size();
				if ( size % 2 == 1 ) // 1, 3, 5
				{
					// we're on correct sequence and same monster, add entry to vector.
					achievementRhythmOfTheKnightVec[player].push_back(std::make_pair(target->ticks, targetUid));
					if ( size == 5 )
					{
						// we pushed back to a total of 6 entries, get achievement.
						real_t timeTaken = (achievementRhythmOfTheKnightVec[player].at(5).first - achievementRhythmOfTheKnightVec[player].at(0).first) / 50.f;
						if ( timeTaken <= 3 )
						{
							//messagePlayer(0, "achievement get!, time taken %f", timeTaken);
							achievementStatusRhythmOfTheKnight[player] = true;
							steamAchievementClient(player, "BARONY_ACH_RHYTHM_OF_THE_KNIGHT");
						}
						achievementRhythmOfTheKnightVec[player].clear();
					}
				}
				else
				{
					// we attacked twice and we're out of sequence.
					achievementRhythmOfTheKnightVec[player].clear();
					//messagePlayer(0, "cleared, out of sequence");
					return;
				}
			}
		}
	}
	else
	{
		// rhythm is initiated on first successful block
		if ( achievementRhythmOfTheKnightVec[player].empty() )
		{
			achievementRhythmOfTheKnightVec[player].push_back(std::make_pair(target->ticks, targetUid));
		}
		else
		{
			if ( achievementRhythmOfTheKnightVec[player].at(0).second != targetUid )
			{
				// check first uid entry, if not matching the monster, we swapped targets and should reset.
				achievementRhythmOfTheKnightVec[player].clear();
				//messagePlayer(0, "cleared, not blocking same target");
			}
			int size = achievementRhythmOfTheKnightVec[player].size();
			if ( size == 1 || size == 3 || size == 5 )
			{
				achievementRhythmOfTheKnightVec[player].clear();
				//messagePlayer(0, "cleared, out of sequence");
			}
			achievementRhythmOfTheKnightVec[player].push_back(std::make_pair(target->ticks, targetUid));
		}
	}
}

void updateAchievementBaitAndSwitch(int player, bool isTeleporting)
{
	if ( player < 0 || player >= MAXPLAYERS )
	{
		return;
	}
	if ( !stats[player] || stats[player]->playerRace != RACE_SUCCUBUS || achievementStatusBaitAndSwitch[player] || multiplayer == CLIENT )
	{
		return;
	}

	if ( stats[player]->playerRace == RACE_SUCCUBUS && stats[player]->appearance != 0 )
	{
		return;
	}

	if ( !isTeleporting )
	{
		achievementBaitAndSwitchTimer[player] = ticks;
	}
	else
	{
		if ( achievementBaitAndSwitchTimer[player] > 0 && (ticks - achievementBaitAndSwitchTimer[player]) <= TICKS_PER_SECOND )
		{
			achievementStatusBaitAndSwitch[player] = true;
			steamAchievementClient(player, "BARONY_ACH_BAIT_AND_SWITCH");
		}
	}
}

void updateAchievementThankTheTank(int player, Entity* target, bool targetKilled)
{
	if ( player < 0 || player >= MAXPLAYERS )
	{
		return;
	}
	if ( achievementStatusThankTheTank[player] || multiplayer == CLIENT )
	{
		return;
	}

	if ( !targetKilled )
	{
		achievementThankTheTankPair[player] = std::make_pair(ticks, target->getUID()); // track the monster UID defending against
		//messagePlayer(0, "pair: %d, %d", achievementThankTheTankPair[player].first, achievementThankTheTankPair[player].second);
	}
	else if ( achievementThankTheTankPair[player].first != 0
		&& achievementThankTheTankPair[player].second != 0 ) // check there is a ticks/UID entry.
	{
		if ( players[player] && players[player]->entity )
		{
			if ( players[player]->entity->checkEnemy(target) )
			{
				if ( target->getUID() == achievementThankTheTankPair[player].second )
				{
					// same target dying, check timestamp within 3 seconds.
					if ( (ticks - achievementThankTheTankPair[player].first) / 50.f < 3.f )
					{
						achievementStatusThankTheTank[player] = true;
					}
				}
			}
		}
	}
}

#ifdef STEAMWORKS

bool steamLeaderboardSetScore(score_t* score)
{
	if ( !g_SteamLeaderboards )
	{
		return false;
	}

	if ( !score )
	{
		return false;
	}

	if ( score->victory == 0 )
	{
		return false;
	}

	if ( score->conductGameChallenges[CONDUCT_CHEATS_ENABLED] 
		|| gamemods_disableSteamAchievements
		|| score->conductGameChallenges[CONDUCT_LIFESAVING] )
	{
		return false;
	}
	if ( score->gameStatistics[STATISTICS_DISABLE_UPLOAD] == 1 )
	{
		return false;
	}
	
	bool monster = false;
	if ( score->stats && score->stats->playerRace > 0 && score->stats->appearance == 0 )
	{
		monster = true;
	}

	if ( !score->conductGameChallenges[CONDUCT_MULTIPLAYER] )
	{
		// single player
		if ( !score->conductGameChallenges[CONDUCT_HARDCORE] )
		{
			if ( score->victory == 2 )
			{
				if ( monster )
				{
					g_SteamLeaderboards->LeaderboardUpload.boardIndex = LEADERBOARD_DLC_HELL_TIME;
				}
				else
				{
					g_SteamLeaderboards->LeaderboardUpload.boardIndex = LEADERBOARD_HELL_TIME;
				}
			}
			else if ( score->victory == 3 )
			{
				if ( monster )
				{
					g_SteamLeaderboards->LeaderboardUpload.boardIndex = LEADERBOARD_DLC_NORMAL_TIME;
				}
				else
				{
					g_SteamLeaderboards->LeaderboardUpload.boardIndex = LEADERBOARD_NORMAL_TIME;
				}
			}
			else if ( score->victory == 1 )
			{
				if ( monster )
				{
					g_SteamLeaderboards->LeaderboardUpload.boardIndex = LEADERBOARD_DLC_CLASSIC_TIME;
				}
				else
				{
					g_SteamLeaderboards->LeaderboardUpload.boardIndex = LEADERBOARD_CLASSIC_TIME;
				}
			}
		}
		else if ( score->conductGameChallenges[CONDUCT_HARDCORE] )
		{
			if ( score->victory == 3 )
			{
				if ( monster )
				{
					g_SteamLeaderboards->LeaderboardUpload.boardIndex = LEADERBOARD_DLC_HARDCORE_TIME;
				}
				else
				{
					g_SteamLeaderboards->LeaderboardUpload.boardIndex = LEADERBOARD_HARDCORE_TIME;
				}
			}
			else
			{
				if ( monster )
				{
					g_SteamLeaderboards->LeaderboardUpload.boardIndex = LEADERBOARD_DLC_CLASSIC_HARDCORE_TIME;
				}
				else
				{
					g_SteamLeaderboards->LeaderboardUpload.boardIndex = LEADERBOARD_CLASSIC_HARDCORE_TIME;
				}
			}
		}
	}
	else if ( score->conductGameChallenges[CONDUCT_MULTIPLAYER] )
	{
		// multiplayer
		if ( score->victory == 2 )
		{
			if ( monster )
			{
				g_SteamLeaderboards->LeaderboardUpload.boardIndex = LEADERBOARD_DLC_MULTIPLAYER_HELL_TIME;
			}
			else
			{
				g_SteamLeaderboards->LeaderboardUpload.boardIndex = LEADERBOARD_MULTIPLAYER_HELL_TIME;
			}
		}
		else if ( score->victory == 3 )
		{
			if ( monster )
			{
				g_SteamLeaderboards->LeaderboardUpload.boardIndex = LEADERBOARD_DLC_MULTIPLAYER_TIME;
			}
			else
			{
				g_SteamLeaderboards->LeaderboardUpload.boardIndex = LEADERBOARD_MULTIPLAYER_TIME;
			}
		}
		else if ( score->victory == 1 )
		{
			if ( monster )
			{
				g_SteamLeaderboards->LeaderboardUpload.boardIndex = LEADERBOARD_DLC_MULTIPLAYER_CLASSIC_TIME;
			}
			else
			{
				g_SteamLeaderboards->LeaderboardUpload.boardIndex = LEADERBOARD_MULTIPLAYER_CLASSIC_TIME;
			}
		}
	}
	else
	{
		g_SteamLeaderboards->LeaderboardUpload.boardIndex = LEADERBOARD_NONE;
	}

	if ( g_SteamLeaderboards->LeaderboardUpload.boardIndex == LEADERBOARD_NONE )
	{
		return false;
	}

	// assemble the score tags.
	//int completionTime = score->completionTime;
	int c = 0;
	int tag = TAG_MONSTER_KILLS_1;
	int i = 0;
	int tagWidth = 8;
	for ( c = 0; c < NUMMONSTERS; ++c )
	{
		g_SteamLeaderboards->LeaderboardUpload.tags[tag] |= (static_cast<Uint8>(score->kills[c]) << (i * tagWidth));
		++i;
		if ( i >((32 / tagWidth) - 1) )
		{
			i = 0;
			++tag;
		}
	}

	i = 0;
	tagWidth = 8;
	tag = TAG_NAME1;
	for ( c = 0; c < std::min(32, (int)(strlen(score->stats->name))); ++c )
	{
		g_SteamLeaderboards->LeaderboardUpload.tags[tag] |= (Uint8)(score->stats->name[c]) << (i * tagWidth);
		++i;
		if ( i > ((32 / tagWidth) - 1) )
		{
			i = 0;
			++tag;
		}
	}

	tagWidth = 8;
	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_RACESEXAPPEARANCECLASS] |= score->stats->type;
	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_RACESEXAPPEARANCECLASS] |= (score->stats->sex) << (tagWidth * 1);
	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_RACESEXAPPEARANCECLASS] |= (score->stats->appearance) << (tagWidth * 2);
	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_RACESEXAPPEARANCECLASS] |= (score->classnum) << (tagWidth * 3);

	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_VICTORYDUNGEONLEVELCONDUCTORIGINAL] |= (score->victory);
	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_VICTORYDUNGEONLEVELCONDUCTORIGINAL] |= (score->dungeonlevel) << (tagWidth);
	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_VICTORYDUNGEONLEVELCONDUCTORIGINAL] |= (score->conductPenniless) << (tagWidth * 2);
	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_VICTORYDUNGEONLEVELCONDUCTORIGINAL] |= (score->conductFoodless) << (tagWidth * 2 + 1);
	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_VICTORYDUNGEONLEVELCONDUCTORIGINAL] |= (score->conductVegetarian) << (tagWidth * 2 + 2);
	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_VICTORYDUNGEONLEVELCONDUCTORIGINAL] |= (score->conductIlliterate) << (tagWidth * 2 + 3);

	tag = TAG_CONDUCT_2W_1;
	tagWidth = 16;
	i = 0;
	for ( c = 0; c < 32; ++c )
	{
		if ( c < 16 )
		{
			g_SteamLeaderboards->LeaderboardUpload.tags[tag] |= score->conductGameChallenges[c] << (c * 2);
		}
		else
		{
			g_SteamLeaderboards->LeaderboardUpload.tags[tag] |= score->conductGameChallenges[c] << ((16 - c) * 2);
		}
		if ( c == 15 )
		{
			++tag;
		}
	}

	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_CONDUCT_4W_1] |= (Uint8)(score->stats->playerRace); // store in right-most 8 bits.
	// conducts TAG_CONDUCT_4W_2 to TAG_CONDUCT_4W_4 unused.

	// store new gameplay stats as required. not many to start with.
	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_GAMEPLAY_STATS_2W_1] |= std::min(3, score->gameStatistics[STATISTICS_FIRE_MAYBE_DIFFERENT]);
	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_GAMEPLAY_STATS_2W_1] |= std::min(3, score->gameStatistics[STATISTICS_SITTING_DUCK]) << 2;
	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_GAMEPLAY_STATS_2W_1] |= std::min(3, score->gameStatistics[STATISTICS_TEMPT_FATE]) << 4;

	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_GAMEPLAY_STATS_8W_1] |= (Uint8)score->gameStatistics[STATISTICS_BOMB_SQUAD];
	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_GAMEPLAY_STATS_8W_1] |= (Uint8)score->gameStatistics[STATISTICS_HOT_TUB_TIME_MACHINE] << 8;
	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_GAMEPLAY_STATS_8W_1] |= (Uint8)score->gameStatistics[STATISTICS_YES_WE_CAN] << 16;

	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_GAMEPLAY_STATS_16W_1] |= (Uint16)score->gameStatistics[STATISTICS_HEAL_BOT];

	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_HEALTH] |= (Uint16)score->stats->MAXHP;
	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_HEALTH] |= (Uint16)score->stats->HP << 16;

	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_MANA] |= (Uint16)score->stats->MAXMP;
	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_MANA] |= (Uint16)score->stats->MP << 16;

	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_STRDEXCONINT] |= (Uint8)score->stats->STR;
	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_STRDEXCONINT] |= (Uint8)score->stats->DEX << 8;
	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_STRDEXCONINT] |= (Uint8)score->stats->CON << 16;
	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_STRDEXCONINT] |= (Uint8)score->stats->INT << 24;

	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_PERCHREXPLVL] |= (Uint8)score->stats->PER;
	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_PERCHREXPLVL] |= (Uint8)score->stats->CHR << 8;
	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_PERCHREXPLVL] |= (Uint8)score->stats->EXP << 16;
	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_PERCHREXPLVL] |= (Uint8)score->stats->LVL << 24;

	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_GOLD] |= score->stats->GOLD;

	tagWidth = 8;
	tag = TAG_PROFICIENCY1;
	i = 0;
	for ( c = 0; c < NUMPROFICIENCIES; ++c )
	{
		g_SteamLeaderboards->LeaderboardUpload.tags[tag] |= score->stats->PROFICIENCIES[c] << (i * tagWidth);
		++i;
		if ( i > ((32 / tagWidth) - 1) )
		{
			i = 0;
			++tag;
		}
	}

	if ( score->stats->helmet )
	{
		g_SteamLeaderboards->LeaderboardUpload.tags[TAG_EQUIPMENT1] |= (Uint8)(score->stats->helmet->type);
		g_SteamLeaderboards->LeaderboardUpload.tags[TAG_EQUIPMENT_BEATITUDE1] |= (Sint16)(score->stats->helmet->beatitude);
		g_SteamLeaderboards->LeaderboardUpload.tags[TAG_EQUIPMENT_APPEARANCE] |=
			(score->stats->helmet->appearance % items[score->stats->helmet->type].variations);
	}
	if ( score->stats->breastplate )
	{
		g_SteamLeaderboards->LeaderboardUpload.tags[TAG_EQUIPMENT1] |= (Uint8)(score->stats->breastplate->type) << 8;
		g_SteamLeaderboards->LeaderboardUpload.tags[TAG_EQUIPMENT_BEATITUDE1] |= (Sint16)(score->stats->breastplate->beatitude) << 8;
		g_SteamLeaderboards->LeaderboardUpload.tags[TAG_EQUIPMENT_APPEARANCE] |=
			(score->stats->breastplate->appearance % items[score->stats->breastplate->type].variations) << 8;
	}
	if ( score->stats->gloves )
	{
		g_SteamLeaderboards->LeaderboardUpload.tags[TAG_EQUIPMENT1] |= (Uint8)(score->stats->gloves->type) << 16;
		g_SteamLeaderboards->LeaderboardUpload.tags[TAG_EQUIPMENT_BEATITUDE1] |= (Sint16)(score->stats->gloves->beatitude) << 16;
	}
	if ( score->stats->shoes )
	{
		g_SteamLeaderboards->LeaderboardUpload.tags[TAG_EQUIPMENT1] |= (Uint8)(score->stats->shoes->type) << 24;
		g_SteamLeaderboards->LeaderboardUpload.tags[TAG_EQUIPMENT_BEATITUDE1] |= (Sint16)(score->stats->shoes->beatitude) << 24;
	}

	if ( score->stats->shield )
	{
		g_SteamLeaderboards->LeaderboardUpload.tags[TAG_EQUIPMENT2] |= (Uint8)(score->stats->shield->type);
		g_SteamLeaderboards->LeaderboardUpload.tags[TAG_EQUIPMENT_BEATITUDE2] |= (Sint16)(score->stats->shield->beatitude);
		g_SteamLeaderboards->LeaderboardUpload.tags[TAG_EQUIPMENT_APPEARANCE] |=
			(score->stats->shield->appearance % items[score->stats->shield->type].variations) << 12;
	}
	if ( score->stats->weapon )
	{
		g_SteamLeaderboards->LeaderboardUpload.tags[TAG_EQUIPMENT2] |= (Uint8)(score->stats->weapon->type) << 8;
		g_SteamLeaderboards->LeaderboardUpload.tags[TAG_EQUIPMENT_BEATITUDE2] |= (Sint16)(score->stats->weapon->beatitude) << 8;
	}
	if ( score->stats->cloak )
	{
		g_SteamLeaderboards->LeaderboardUpload.tags[TAG_EQUIPMENT2] |= (Uint8)(score->stats->cloak->type) << 16;
		g_SteamLeaderboards->LeaderboardUpload.tags[TAG_EQUIPMENT_BEATITUDE2] |= (Sint16)(score->stats->cloak->beatitude) << 16;
		g_SteamLeaderboards->LeaderboardUpload.tags[TAG_EQUIPMENT_APPEARANCE] |= 
			(score->stats->cloak->appearance % items[score->stats->cloak->type].variations) << 4;
	}
	if ( score->stats->amulet )
	{
		g_SteamLeaderboards->LeaderboardUpload.tags[TAG_EQUIPMENT2] |= (Uint8)(score->stats->amulet->type) << 24;
		g_SteamLeaderboards->LeaderboardUpload.tags[TAG_EQUIPMENT_BEATITUDE2] |= (Sint16)(score->stats->amulet->beatitude) << 16;
	}

	if ( score->stats->ring )
	{
		g_SteamLeaderboards->LeaderboardUpload.tags[TAG_EQUIPMENT3] |= (Uint8)(score->stats->ring->type);
		g_SteamLeaderboards->LeaderboardUpload.tags[TAG_EQUIPMENT_BEATITUDE3] |= (Sint16)(score->stats->ring->beatitude);
	}
	if ( score->stats->mask )
	{
		g_SteamLeaderboards->LeaderboardUpload.tags[TAG_EQUIPMENT3] |= (Uint8)(score->stats->mask->type) << 8;
		g_SteamLeaderboards->LeaderboardUpload.tags[TAG_EQUIPMENT_BEATITUDE3] |= (Sint16)(score->stats->mask->beatitude);
	}

	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_TOTAL_SCORE] = totalScore(score);
	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_COMPLETION_TIME] = score->completionTime / TICKS_PER_SECOND;
	return true;
}

bool steamLeaderboardReadScore(int tags[CSteamLeaderboards::k_numLeaderboardTags])
{
	stats[0]->clearStats();

	int c = 0;
	int tag = TAG_MONSTER_KILLS_1;
	int tagWidth = 8;
	int i = 0;
	for ( c = 0; c < NUMMONSTERS; c++ )
	{
		kills[c] = ((tags[tag]) >> (i * tagWidth)) & 0xFF;
		++i;
		if ( i > ((32 / tagWidth) - 1) )
		{
			i = 0;
			++tag;
		}
	}

	i = 0;
	tagWidth = 8;
	tag = TAG_NAME1;
	char name[33] = "";
	for ( c = 0; c < 32; ++c )
	{
		name[c] = ((tags[tag]) >> (i * tagWidth)) & 0xFF;
		if ( name[c] == 0 )
		{
			break;
		}
		++i;
		if ( i > ((32 / tagWidth) - 1) )
		{
			i = 0;
			++tag;
		}
	}
	name[c] = '\0';
	strcpy(stats[0]->name, name);


	tagWidth = 8;
	stats[0]->type = (Monster)(tags[TAG_RACESEXAPPEARANCECLASS] & 0xFF);
	stats[0]->sex = (sex_t)((tags[TAG_RACESEXAPPEARANCECLASS] >> tagWidth) & 0xFF);
	stats[0]->appearance = (tags[TAG_RACESEXAPPEARANCECLASS] >> tagWidth * 2) & 0xFF;
	client_classes[0] = (tags[TAG_RACESEXAPPEARANCECLASS] >> tagWidth * 3) & 0xFF;

	victory = (tags[TAG_VICTORYDUNGEONLEVELCONDUCTORIGINAL] >> tagWidth * 0) & 0xFF;
	currentlevel = (tags[TAG_VICTORYDUNGEONLEVELCONDUCTORIGINAL] >> tagWidth * 1) & 0xFF;
	conductPenniless = (tags[TAG_VICTORYDUNGEONLEVELCONDUCTORIGINAL] >> tagWidth * 2) & 1;
	conductFoodless = (tags[TAG_VICTORYDUNGEONLEVELCONDUCTORIGINAL] >> (tagWidth * 2 + 1)) & 1;
	conductVegetarian = (tags[TAG_VICTORYDUNGEONLEVELCONDUCTORIGINAL] >> (tagWidth * 2 + 2)) & 1;
	conductIlliterate = (tags[TAG_VICTORYDUNGEONLEVELCONDUCTORIGINAL] >> (tagWidth * 2 + 3)) & 1;

	tag = TAG_CONDUCT_2W_1;
	tagWidth = 2;
	i = 0;
	for ( c = 0; c < 32; ++c )
	{
		if ( c < 16 )
		{
			conductGameChallenges[c] = (tags[tag] >> c * tagWidth) & 0b11;
		}
		else
		{
			conductGameChallenges[c] = (tags[tag] >> (16 - c) * tagWidth) & 0b11;
		}
		if ( c == 15 )
		{
			++tag;
		}
	}

	stats[0]->playerRace = (tags[TAG_CONDUCT_4W_1] & 0xFF);

	// conducts TAG_CONDUCT_4W_2 to TAG_CONDUCT_4W_4 unused.

	// store new gameplay stats as required. not many to start with.
	gameStatistics[STATISTICS_FIRE_MAYBE_DIFFERENT] = tags[TAG_GAMEPLAY_STATS_2W_1] & 0b11;
	gameStatistics[STATISTICS_SITTING_DUCK] = (tags[TAG_GAMEPLAY_STATS_2W_1] >> 2) & 0b11;
	gameStatistics[STATISTICS_TEMPT_FATE] = (tags[TAG_GAMEPLAY_STATS_2W_1] >> 4) & 0b11;

	gameStatistics[STATISTICS_BOMB_SQUAD] = tags[TAG_GAMEPLAY_STATS_8W_1] & 0xFF;
	gameStatistics[STATISTICS_HOT_TUB_TIME_MACHINE] = (tags[TAG_GAMEPLAY_STATS_8W_1] >> 8) & 0xFF;
	gameStatistics[STATISTICS_YES_WE_CAN] = (tags[TAG_GAMEPLAY_STATS_8W_1] >> 16) & 0xFF;

	gameStatistics[STATISTICS_HEAL_BOT] = tags[TAG_GAMEPLAY_STATS_16W_1] & 0xFFFF;

	stats[0]->MAXHP = tags[TAG_HEALTH] & 0xFFFF;
	stats[0]->HP = (tags[TAG_HEALTH] >> 16) & 0xFFFF;
	stats[0]->MAXMP = tags[TAG_MANA] & 0xFFFF;
	stats[0]->MP = (tags[TAG_MANA] >> 16) & 0xFFFF;
	stats[0]->STR = (tags[TAG_STRDEXCONINT] >> 0) & 0xFF;
	if ( stats[0]->STR > 240 )
	{
		stats[0]->STR = (Sint8)stats[0]->STR;
	}
	stats[0]->DEX = (tags[TAG_STRDEXCONINT] >> 8) & 0xFF;
	if ( stats[0]->DEX > 240 )
	{
		stats[0]->DEX = (Sint8)stats[0]->DEX;
	}
	stats[0]->CON = (tags[TAG_STRDEXCONINT] >> 16) & 0xFF;
	if ( stats[0]->CON > 240 )
	{
		stats[0]->CON = (Sint8)stats[0]->CON;
	}
	stats[0]->INT = (tags[TAG_STRDEXCONINT] >> 24) & 0xFF;
	if ( stats[0]->INT > 240 )
	{
		stats[0]->INT = (Sint8)stats[0]->INT;
	}
	stats[0]->PER = (tags[TAG_PERCHREXPLVL] >> 0) & 0xFF;
	if ( stats[0]->PER > 240 )
	{
		stats[0]->PER = (Sint8)stats[0]->PER;
	}
	stats[0]->CHR = (tags[TAG_PERCHREXPLVL] >> 8) & 0xFF;
	if ( stats[0]->CHR > 240 )
	{
		stats[0]->CHR = (Sint8)stats[0]->CHR;
	}
	stats[0]->EXP = (tags[TAG_PERCHREXPLVL] >> 16) & 0xFF;
	stats[0]->LVL = (tags[TAG_PERCHREXPLVL] >> 24) & 0xFF;
	stats[0]->GOLD = tags[TAG_GOLD];

	tagWidth = 8;
	tag = TAG_PROFICIENCY1;
	i = 0;
	for ( c = 0; c < NUMPROFICIENCIES; ++c )
	{
		stats[0]->PROFICIENCIES[c] = (tags[tag] >> (i * tagWidth)) & 0xFF;
		++i;
		if ( i > ((32 / tagWidth) - 1) )
		{
			i = 0;
			++tag;
		}
	}

	list_FreeAll(&stats[0]->inventory);
	if ( ((tags[TAG_EQUIPMENT1] >> 0) & 0xFF) > 0 )
	{
		stats[0]->helmet = newItem(ItemType((tags[TAG_EQUIPMENT1] >> 0) & 0xFF), EXCELLENT, 
			Sint16((tags[TAG_EQUIPMENT_BEATITUDE1] >> 0) & 0xFF), 1, tags[TAG_EQUIPMENT_APPEARANCE] & 0xF, true, &stats[0]->inventory);
	}
	if ( ((tags[TAG_EQUIPMENT1] >> 8) & 0xFF) > 0 )
	{
		stats[0]->breastplate = newItem(ItemType((tags[TAG_EQUIPMENT1] >> 8) & 0xFF), EXCELLENT,
			Sint16((tags[TAG_EQUIPMENT_BEATITUDE1] >> 8) & 0xFF), 1, (tags[TAG_EQUIPMENT_APPEARANCE] >> 8) & 0xF, true, &stats[0]->inventory);
	}
	if ( ((tags[TAG_EQUIPMENT1] >> 16) & 0xFF) > 0 )
	{
		stats[0]->gloves = newItem(ItemType((tags[TAG_EQUIPMENT1] >> 16) & 0xFF), EXCELLENT, Sint16((tags[TAG_EQUIPMENT_BEATITUDE1] >> 16) & 0xFF), 1, rand(), true, &stats[0]->inventory);
	}
	if ( ((tags[TAG_EQUIPMENT1] >> 24) & 0xFF) > 0 )
	{
		stats[0]->shoes = newItem(ItemType((tags[TAG_EQUIPMENT1] >> 24) & 0xFF), EXCELLENT, Sint16((tags[TAG_EQUIPMENT_BEATITUDE1] >> 24) & 0xFF), 1, rand(), true, &stats[0]->inventory);
	}

	if ( ((tags[TAG_EQUIPMENT2] >> 0) & 0xFF) > 0 )
	{
		stats[0]->shield = newItem(ItemType((tags[TAG_EQUIPMENT2] >> 0) & 0xFF), EXCELLENT, 
			Sint16((tags[TAG_EQUIPMENT_BEATITUDE2] >> 0) & 0xFF), 1, (tags[TAG_EQUIPMENT_APPEARANCE] >> 12) & 0xF, true, &stats[0]->inventory);
	}
	if ( ((tags[TAG_EQUIPMENT2] >> 8) & 0xFF) > 0 )
	{
		stats[0]->weapon = newItem(ItemType((tags[TAG_EQUIPMENT2] >> 8) & 0xFF), EXCELLENT, Sint16((tags[TAG_EQUIPMENT_BEATITUDE2] >> 8) & 0xFF), 1, rand(), true, &stats[0]->inventory);
	}
	if ( ((tags[TAG_EQUIPMENT2] >> 16) & 0xFF) > 0 )
	{
		stats[0]->cloak = newItem(ItemType((tags[TAG_EQUIPMENT2] >> 16) & 0xFF), EXCELLENT, 
			Sint16((tags[TAG_EQUIPMENT_BEATITUDE2] >> 16) & 0xFF), 1, (tags[TAG_EQUIPMENT_APPEARANCE] >> 4) & 0xF, true, &stats[0]->inventory);
	}
	if ( ((tags[TAG_EQUIPMENT2] >> 24) & 0xFF) > 0 )
	{
		stats[0]->amulet = newItem(ItemType((tags[TAG_EQUIPMENT2] >> 24) & 0xFF), EXCELLENT, Sint16((tags[TAG_EQUIPMENT_BEATITUDE2] >> 24) & 0xFF), 1, rand(), true, &stats[0]->inventory);
	}

	if ( ((tags[TAG_EQUIPMENT3] >> 16) & 0xFF) > 0 )
	{
		stats[0]->ring = newItem(ItemType((tags[TAG_EQUIPMENT3] >> 16) & 0xFF), EXCELLENT, Sint16((tags[TAG_EQUIPMENT_BEATITUDE3] >> 0) & 0xFF), 1, rand(), true, &stats[0]->inventory);
	}
	if ( ((tags[TAG_EQUIPMENT3] >> 24) & 0xFF) > 0 )
	{
		stats[0]->mask = newItem(ItemType((tags[TAG_EQUIPMENT3] >> 24) & 0xFF), EXCELLENT, Sint16((tags[TAG_EQUIPMENT_BEATITUDE3] >> 8) & 0xFF), 1, rand(), true, &stats[0]->inventory);
	}

	completionTime = tags[TAG_COMPLETION_TIME] * TICKS_PER_SECOND;
	//g_SteamLeaderboards->LeaderboardUpload.tags[TAG_TOTAL_SCORE] = totalScore(score);
	return true;
}

#endif // STEAMWORKS

void AchievementObserver::getCurrentPlayerUids()
{
	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		if ( players[i] && players[i]->entity )
		{
			playerUids[i] = players[i]->entity->getUID();
		}
	}
}

bool AchievementObserver::updateOnLevelChange()
{
	if ( levelObserved != currentlevel )
	{
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			playerAchievements[i].flutterShyCoordinates = std::make_pair(0.0, 0.0);
			playerAchievements[i].caughtInAMoshTargets.clear();
			playerAchievements[i].strungOutTicks.clear();
			playerAchievements[i].ironicPunishmentTargets.clear();
			playerAchievements[i].gastricBypassSpell = std::make_pair(0, 0);
			playerAchievements[i].rat5000secondRule.clear();
		}
		levelObserved = currentlevel;
		return true;
	}
	return false;
}

int AchievementObserver::checkUidIsFromPlayer(Uint32 uid)
{
	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		if ( achievementObserver.playerUids[i] == uid )
		{
			return i;
		}
	}
	return -1;
}

void AchievementObserver::updateData()
{
	getCurrentPlayerUids();
	if ( updateOnLevelChange() )
	{
		entityAchievementsToProcess.clear();
#ifdef DEBUG_ACHIEVEMENTS
		messagePlayer(0, "[DEBUG]: Achievement data reset for floor.");
#endif
	}
}

bool AchievementObserver::addEntityAchievementTimer(Entity* entity, int achievement, int ticks, bool resetTimerIfActive, int optionalIncrement)
{
	if ( !entity )
	{
		return false;
	}
	Uint32 uid = entity->getUID();
	if ( uid == 0 )
	{
		return false;
	}

	auto it = entityAchievementsToProcess.find(uid);
	if ( it != entityAchievementsToProcess.end() )
	{
		auto inner_it = (*it).second.find(achievement);
		if ( inner_it != (*it).second.end() )
		{
			//achievement exists, need to update the ticks value.
			if ( resetTimerIfActive )
			{
				entityAchievementsToProcess[uid][achievement].first = ticks;
			}
			entityAchievementsToProcess[uid][achievement].second += optionalIncrement;
			return false;
		}
		else
		{
			//uid exists, but achievement is not in map. make entry.
			entityAchievementsToProcess[uid].insert(std::make_pair(achievement, std::make_pair(ticks, optionalIncrement))); // set the inner map properties.
			return true;
		}
	}
	else
	{
		// uid does not exist in map, make new entry.
		entityAchievementsToProcess.insert(std::make_pair(uid, std::unordered_map<int, std::pair<int, int>>())); // add a map object at the first key.
		entityAchievementsToProcess[uid].insert(std::make_pair(achievement, std::make_pair(ticks, optionalIncrement))); // set the inner map properties.
		return true;
	}
}

void AchievementObserver::printActiveAchievementTimers()
{
	for ( auto it = entityAchievementsToProcess.begin(); it != entityAchievementsToProcess.end(); ++it )
	{
		for ( auto inner_it = (*it).second.begin(); inner_it != (*it).second.end(); ++inner_it )
		{
			messagePlayer(0, "Uid: %d, achievement: %d, ticks: %d, counter: %d", (*it).first, (*inner_it).first, (*inner_it).second.first, (*inner_it).second.second);
		}
	}
}

void AchievementObserver::achievementTimersTickDown()
{
	for ( auto it = entityAchievementsToProcess.begin(); it != entityAchievementsToProcess.end(); /* don't iterate here*/ )
	{
		for ( auto inner_it = (*it).second.begin(); inner_it != (*it).second.end(); /* don't iterate here*/ )
		{
			bool removeEntry = false;
			if ( (*inner_it).second.first > 0 )
			{
				--((*inner_it).second.first);
				if ( (*inner_it).second.first == 0 )
				{
					// remove me.
					removeEntry = true;
				}
			}
			else if ( (*inner_it).second.first == 0 )
			{
				// remove me.
				removeEntry = true;
			}

			if ( removeEntry )
			{
				inner_it = (*it).second.erase(inner_it);
			}
			else
			{
				++inner_it;
			}
		}

		if ( (*it).second.empty() )
		{
			it = entityAchievementsToProcess.erase(it);
		}
		else
		{
			++it;
		}
	}

	//printActiveAchievementTimers();
}

void AchievementObserver::awardAchievementIfActive(int player, Entity* entity, int achievement)
{
	if ( !entity )
	{
		return;
	}
	Uint32 uid = entity->getUID();
	auto it = entityAchievementsToProcess.find(uid);
	if ( it != entityAchievementsToProcess.end() )
	{
		auto inner_it = (*it).second.find(achievement);
		if ( inner_it != (*it).second.end() && (*it).second[achievement].first != 0 )
		{
			if ( achievement == BARONY_ACH_BOMBTRACK )
			{
				if ( (*it).second[achievement].second >= 5 )
				{
					awardAchievement(player, achievement);
				}
			}
			else if ( achievement == BARONY_ACH_OHAI_MARK )
			{
				serverUpdatePlayerGameplayStats(player, STATISTICS_OHAI_MARK, 1);
			}
			else if ( achievement == BARONY_ACH_IF_YOU_LOVE_SOMETHING )
			{
				playerAchievements[player].ifYouLoveSomething++;
			}
			else if ( achievement == BARONY_ACH_COWBOY_FROM_HELL )
			{
				steamStatisticUpdateClient(player, STEAM_STAT_COWBOY_FROM_HELL, STEAM_STAT_INT, 1);
			}
			else
			{
				awardAchievement(player, achievement);
			}
		}
	}
}

void AchievementObserver::checkMapScriptsOnVariableSet()
{
	for ( auto it = textSourceScript.scriptVariables.begin(); it != textSourceScript.scriptVariables.end(); ++it )
	{
		size_t found = (*it).first.find("$ACH_TUTORIAL_SECRET");
		if ( found != std::string::npos )
		{
			std::string mapname = map.name;
			if ( mapname.find("Tutorial Hub") != std::string::npos )
			{
				updatePlayerAchievement(clientnum, BARONY_ACH_MASTER, EXTRA_CREDIT_SECRET);
			}
			else
			{
				updatePlayerAchievement(clientnum, BARONY_ACH_EXTRA_CREDIT, EXTRA_CREDIT_SECRET);
			}
		}
	}
}

void AchievementObserver::updatePlayerAchievement(int player, Achievement achievement, AchievementEvent achEvent)
{
	switch ( achievement )
	{
		case BARONY_ACH_BACK_TO_BASICS:
			if ( gameModeManager.getMode() == GameModeManager_t::GAME_MODE_TUTORIAL )
			{
				bool alternateUnlock = false;
#ifdef STEAMWORKS
				if ( SteamUser()->BLoggedOn() )
				{
					SteamUserStats()->GetAchievement("BARONY_ACH_LICH_HUNTER", &alternateUnlock);
				}
#endif
				if ( g_SteamStats[STEAM_STAT_BACK_TO_BASICS].m_iValue == 1 || alternateUnlock )
				{
					awardAchievement(player, achievement);
				}
			}
			break;
		case BARONY_ACH_FAST_LEARNER:
			if ( gameModeManager.getMode() == GameModeManager_t::GAME_MODE_TUTORIAL )
			{
				if ( g_SteamStats[STEAM_STAT_DIPLOMA].m_iValue == 10 
					&& gameModeManager.Tutorial.levels.size() == (gameModeManager.Tutorial.getNumTutorialLevels() + 1) ) // include the + 1 for hub
				{
					Uint32 totalTime = 0;
					bool allLevelsCompleted = true;
					for ( auto it = gameModeManager.Tutorial.levels.begin(); it != gameModeManager.Tutorial.levels.end(); ++it )
					{
						if ( it->completionTime > 0 )
						{
							totalTime += it->completionTime;
						}
						else
						{
							if ( it->filename.compare("tutorial_hub") != 0 )
							{
								allLevelsCompleted = false;
								break;
							}
						}
					}
					if ( allLevelsCompleted && totalTime < TICKS_PER_SECOND * 20 * 60 )
					{
						awardAchievement(player, achievement);
					}
				}
			}
			break;
		case BARONY_ACH_MASTER:
			if ( gameModeManager.getMode() == GameModeManager_t::GAME_MODE_TUTORIAL )
			{
				std::string mapname = map.name;
				if ( mapname.find("Tutorial Hub") != std::string::npos )
				{
					awardAchievement(player, achievement);
				}
			}
			break;
		case BARONY_ACH_DIPLOMA:
		case BARONY_ACH_EXTRA_CREDIT:
			if ( gameModeManager.getMode() == GameModeManager_t::GAME_MODE_TUTORIAL )
			{
				std::string mapname = map.name;
				if ( mapname.find("Tutorial Hub") == std::string::npos
					&& mapname.find("Tutorial ") != std::string::npos )
				{
					int levelnum = stoi(mapname.substr(mapname.find("Tutorial ") + strlen("Tutorial "), 2));
					SteamStatIndexes statBitCounter = STEAM_STAT_DIPLOMA_LVLS;
					SteamStatIndexes statLevelTotal = STEAM_STAT_DIPLOMA;
					if ( achievement == BARONY_ACH_EXTRA_CREDIT )
					{
						statBitCounter = STEAM_STAT_EXTRA_CREDIT_LVLS;
						statLevelTotal = STEAM_STAT_EXTRA_CREDIT;
					}

					if ( levelnum >= 1 && levelnum <= gameModeManager.Tutorial.getNumTutorialLevels() )
					{
						if ( !(g_SteamStats[statBitCounter].m_iValue & (1 << levelnum - 1)) ) // bit not set
						{
							// update with the difference in values.
							steamStatisticUpdate(statBitCounter, STEAM_STAT_INT, (1 << levelnum - 1));
						}

						int levelsCompleted = 0;
						for ( int i = 0; i < gameModeManager.Tutorial.getNumTutorialLevels(); ++i )
						{
							if ( g_SteamStats[statBitCounter].m_iValue & (1 << i) ) // count the bits
							{
								++levelsCompleted;
							}
						}
						if ( levelsCompleted >= g_SteamStats[statLevelTotal].m_iValue )
						{
							steamStatisticUpdate(statLevelTotal, STEAM_STAT_INT, levelsCompleted - g_SteamStats[statLevelTotal].m_iValue);
						}
					}
				}
			}
			break;
		case BARONY_ACH_REAL_BOY:
			if ( achEvent == REAL_BOY_HUMAN_RECRUIT )
			{
				playerAchievements[player].realBoy.first = 1;
			}
			else if ( achEvent == REAL_BOY_SHOP )
			{
				playerAchievements[player].realBoy.second = 1;
			}
			if ( playerAchievements[player].realBoy.first == 1 && playerAchievements[player].realBoy.second == 1 )
			{
				awardAchievement(player, achievement);
			}
			break;
		case BARONY_ACH_STRUNG_OUT:
			if ( !playerAchievements[player].strungOut )
			{
				playerAchievements[player].strungOutTicks.push_back(ticks);
				if ( playerAchievements[player].strungOutTicks.size() >= 10 )
				{
					Uint32 timePassed = playerAchievements[player].strungOutTicks.at(playerAchievements[player].strungOutTicks.size() - 1);
					timePassed -= playerAchievements[player].strungOutTicks.at(0);
					if ( timePassed < 9 * TICKS_PER_SECOND )
					{
						playerAchievements[player].strungOut = true;
						awardAchievement(player, achievement);
					}

					while ( playerAchievements[player].strungOutTicks.size() >= 10 )
					{
						auto it = playerAchievements[player].strungOutTicks.begin();
						playerAchievements[player].strungOutTicks.erase(it);
					}
				}
			}
			break;
		case BARONY_ACH_COOP_ESCAPE_MINES:
		{
			std::unordered_set<int> races;
			std::unordered_set<int> classes;
			for ( int i = 0; i < MAXPLAYERS; ++i )
			{
				if ( !client_disconnected[i] )
				{
					if ( stats[i] && stats[i]->playerRace != RACE_HUMAN && stats[i]->appearance == 0 )
					{
						races.insert(stats[i]->playerRace);
					}
					if ( client_classes[i] > CLASS_MONK )
					{
						classes.insert(client_classes[i]);
					}
				}
			}
			std::vector<int> awardAchievementsToAllPlayers;
			if ( !races.empty() )
			{
				if ( races.find(RACE_INCUBUS) != races.end() && races.find(RACE_SUCCUBUS) != races.end() )
				{
					awardAchievementsToAllPlayers.push_back(BARONY_ACH_SWINGERS);
				}
				if ( races.find(RACE_VAMPIRE) != races.end() && races.find(RACE_INSECTOID) != races.end() )
				{
					awardAchievementsToAllPlayers.push_back(BARONY_ACH_COLD_BLOODED);
				}
				if ( races.find(RACE_AUTOMATON) != races.end() && races.find(RACE_SKELETON) != races.end() )
				{
					awardAchievementsToAllPlayers.push_back(BARONY_ACH_SOULLESS);
				}
				if ( races.find(RACE_GOATMAN) != races.end() && races.find(RACE_GOBLIN) != races.end() )
				{
					awardAchievementsToAllPlayers.push_back(BARONY_ACH_TRIBAL);
				}
			}

			if ( !classes.empty() )
			{
				if ( classes.find(CLASS_MACHINIST) != classes.end() && classes.find(CLASS_MESMER) != classes.end() )
				{
					awardAchievementsToAllPlayers.push_back(BARONY_ACH_MANAGEMENT_TEAM);
				}
				if ( classes.find(CLASS_ACCURSED) != classes.end() && classes.find(CLASS_PUNISHER) != classes.end() )
				{
					awardAchievementsToAllPlayers.push_back(BARONY_ACH_SOCIOPATHS);
				}
				if ( classes.find(CLASS_SHAMAN) != classes.end() && classes.find(CLASS_CONJURER) != classes.end() )
				{
					awardAchievementsToAllPlayers.push_back(BARONY_ACH_FACES_OF_DEATH);
				}
				if ( classes.find(CLASS_HUNTER) != classes.end() && classes.find(CLASS_BREWER) != classes.end() )
				{
					awardAchievementsToAllPlayers.push_back(BARONY_ACH_SURVIVALISTS);
				}
			}
			if ( !awardAchievementsToAllPlayers.empty() )
			{
				for ( auto it = awardAchievementsToAllPlayers.begin(); it != awardAchievementsToAllPlayers.end(); ++it )
				{
					for ( int i = 0; i < MAXPLAYERS; ++i )
					{
						if ( !client_disconnected[i] )
						{
							awardAchievement(i, *it);
						}
					}
				}
			}
		}
			break;
		default:
			break;
	}
#ifdef DEBUG_ACHIEVEMENTS
	messagePlayer(player, "[DEBUG]: Processed achievement %d, event: %d", achievement, achEvent);
#endif
}

void AchievementObserver::clearPlayerAchievementData()
{
	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		playerAchievements[i].caughtInAMosh = false;
		playerAchievements[i].bombTrack = false;
		playerAchievements[i].calmLikeABomb = false;
		playerAchievements[i].strungOut = false;
		playerAchievements[i].wonderfulToys = false;
		playerAchievements[i].levitantLackey = false;
		playerAchievements[i].flutterShy = false;
		playerAchievements[i].gastricBypass = false;
		playerAchievements[i].ticksSpentOverclocked = 0;
		playerAchievements[i].tradition = false;
		playerAchievements[i].traditionKills = 0;
		playerAchievements[i].torchererScrap = 0;
		playerAchievements[i].superShredder = 0;
		playerAchievements[i].fixerUpper = 0;
		playerAchievements[i].ifYouLoveSomething = 0;
		playerAchievements[i].socialButterfly = 0;
		playerAchievements[i].rollTheBones = 0;
		playerAchievements[i].trashCompactor = 0;

		playerAchievements[i].realBoy = std::make_pair(0, 0);
		playerAchievements[i].caughtInAMoshTargets.clear();
		playerAchievements[i].strungOutTicks.clear();
		playerAchievements[i].ironicPunishmentTargets.clear();
		playerAchievements[i].flutterShyCoordinates = std::make_pair(0.0, 0.0);
		playerAchievements[i].gastricBypassSpell = std::make_pair(0, 0);
		playerAchievements[i].rat5000secondRule.clear();
	}
}

void AchievementObserver::awardAchievement(int player, int achievement)
{
	switch ( achievement )
	{
		case BARONY_ACH_MASTER:
			steamAchievementClient(player, "BARONY_ACH_MASTER");
			break;
		case BARONY_ACH_FAST_LEARNER:
			steamAchievementClient(player, "BARONY_ACH_FAST_LEARNER");
			break;
		case BARONY_ACH_BACK_TO_BASICS:
			steamAchievementClient(player, "BARONY_ACH_BACK_TO_BASICS");
			break;
		case BARONY_ACH_TELEFRAG:
			steamAchievementClient(player, "BARONY_ACH_TELEFRAG");
			break;
		case BARONY_ACH_REAL_BOY:
			steamAchievementClient(player, "BARONY_ACH_REAL_BOY");
			break;
		case BARONY_ACH_SWINGERS:
			steamAchievementClient(player, "BARONY_ACH_SWINGERS");
			break;
		case BARONY_ACH_COLD_BLOODED:
			steamAchievementClient(player, "BARONY_ACH_COLD_BLOODED");
			break;
		case BARONY_ACH_SOULLESS:
			steamAchievementClient(player, "BARONY_ACH_SOULLESS");
			break;
		case BARONY_ACH_TRIBAL:
			steamAchievementClient(player, "BARONY_ACH_TRIBAL");
			break;
		case BARONY_ACH_MANAGEMENT_TEAM:
			steamAchievementClient(player, "BARONY_ACH_MANAGEMENT_TEAM");
			break;
		case BARONY_ACH_SOCIOPATHS:
			steamAchievementClient(player, "BARONY_ACH_SOCIOPATHS");
			break;
		case BARONY_ACH_FACES_OF_DEATH:
			steamAchievementClient(player, "BARONY_ACH_FACES_OF_DEATH");
			break;
		case BARONY_ACH_SURVIVALISTS:
			steamAchievementClient(player, "BARONY_ACH_SURVIVALISTS");
			break;
		case BARONY_ACH_BOMBTRACK:
			steamAchievementClient(player, "BARONY_ACH_BOMBTRACK");
			break;
		case BARONY_ACH_CALM_LIKE_A_BOMB:
			achievementObserver.playerAchievements[player].calmLikeABomb = true;
			steamAchievementClient(player, "BARONY_ACH_CALM_LIKE_A_BOMB");
			break;
		case BARONY_ACH_CAUGHT_IN_A_MOSH:
			steamAchievementClient(player, "BARONY_ACH_CAUGHT_IN_A_MOSH");
			break;
		case BARONY_ACH_STRUNG_OUT:
			steamAchievementClient(player, "BARONY_ACH_STRUNG_OUT");
			break;
		case BARONY_ACH_PLEASE_HOLD:
			steamAchievementClient(player, "BARONY_ACH_PLEASE_HOLD");
			break;
		case BARONY_ACH_FELL_BEAST:
			steamAchievementClient(player, "BARONY_ACH_FELL_BEAST");
			break;
		case BARONY_ACH_IRONIC_PUNISHMENT:
			steamAchievementClient(player, "BARONY_ACH_IRONIC_PUNISHMENT");
			break;
		default:
			messagePlayer(player, "[WARNING]: Unhandled achievement: %d", achievement);
			break;
	}
}

bool AchievementObserver::PlayerAchievements::checkPathBetweenObjects(Entity* player, Entity* target, int achievement)
{
	if ( !player )
	{
		return false;
	}

	real_t oldx = 0.0;
	real_t oldy = 0.0;
	if ( achievement == BARONY_ACH_LEVITANT_LACKEY )
	{
		if ( this->levitantLackey )
		{
			return false;
		}
	}
	else if ( achievement == BARONY_ACH_WONDERFUL_TOYS )
	{
		if ( this->wonderfulToys )
		{
			return false;
		}
	}
	else if ( achievement == BARONY_ACH_FLUTTERSHY )
	{
		if ( this->flutterShy )
		{
			return false;
		}

		// if the player exists, we need another entity to path to (target is nullptr)
		// use a bodypart entity
		if ( !target )
		{
			target = player->bodyparts.at(0);
			oldx = target->x;
			oldy = target->y;
		}
		target->x = this->flutterShyCoordinates.first;
		target->y = this->flutterShyCoordinates.second;
	}

	if ( !target )
	{
		return false;
	}

	list_t* playerPath = generatePath((int)floor(player->x / 16), (int)floor(player->y / 16),
		(int)floor(target->x / 16), (int)floor(target->y / 16), player, target, true);
	if ( playerPath == nullptr )
	{
		// no path.
		if ( achievement == BARONY_ACH_LEVITANT_LACKEY )
		{
			steamAchievementClient(player->skill[2], "BARONY_ACH_LEVITANT_LACKEY");
			this->levitantLackey = true;
		}
		else if ( achievement == BARONY_ACH_WONDERFUL_TOYS )
		{
			steamAchievementClient(player->skill[2], "BARONY_ACH_WONDERFUL_TOYS");
			this->wonderfulToys = true;
		}
		else if ( achievement == BARONY_ACH_FLUTTERSHY )
		{
			steamAchievementClient(player->skill[2], "BARONY_ACH_FLUTTERSHY");
			this->flutterShy = true;
			target->x = oldx;
			target->y = oldy;
		}
		return true;
	}
	else
	{
		list_FreeAll(playerPath);
		free(playerPath);
		if ( achievement == BARONY_ACH_FLUTTERSHY )
		{
			target->x = oldx;
			target->y = oldy;
		}
	}
	return false;
}

bool AchievementObserver::PlayerAchievements::checkTraditionKill(Entity* player, Entity* target)
{
	if ( tradition )
	{
		return false;
	}

	std::vector<list_t*> entLists = TileEntityList.getEntitiesWithinRadiusAroundEntity(target, 3);
	bool foundFountain = false;
	for ( std::vector<list_t*>::iterator it = entLists.begin(); it != entLists.end(); ++it )
	{
		list_t* currentList = *it;
		node_t* node;
		for ( node = currentList->first; node != nullptr; node = node->next )
		{
			Entity* entity = (Entity*)node->element;
			if ( entity && entity->behavior == &actFountain )
			{
				if ( entityDist(target, entity) < 16 * 3 )
				{
					foundFountain = true;
					break;
				}
			}
		}
		if ( foundFountain )
		{
			break;
		}
	}

	if ( foundFountain )
	{
		steamStatisticUpdateClient(player->skill[2], STEAM_STAT_TRADITION, STEAM_STAT_INT, 1);
		++traditionKills;
		if ( traditionKills >= 25 ) // max is 20, but allow some error in transmission
		{
			tradition = true;
		}
	}
	return true;
}

void AchievementObserver::updateGlobalStat(int index, int value)
{
	if ( multiplayer == CLIENT )
	{
		return;
	}
	if ( conductGameChallenges[CONDUCT_CHEATS_ENABLED]
		|| gamemods_disableSteamAchievements )
	{
		return;
	}
#if defined USE_EOS
	EOS.queueGlobalStatUpdate(index, value);
#endif
}

SteamGlobalStatIndexes getIndexForDeathType(int type)
{
	switch ( static_cast<Monster>(type) )
	{
		case HUMAN:
			return STEAM_GSTAT_DEATHS_HUMAN;
		case RAT:
			return STEAM_GSTAT_DEATHS_RAT;
		case GOBLIN:
			return STEAM_GSTAT_DEATHS_GOBLIN;
		case SLIME:
			return STEAM_GSTAT_DEATHS_SLIME;
		case TROLL:
			return STEAM_GSTAT_DEATHS_TROLL;
		case SPIDER:
			return STEAM_GSTAT_DEATHS_SPIDER;
		case GHOUL:
			return STEAM_GSTAT_DEATHS_GHOUL;
		case SKELETON:
			return STEAM_GSTAT_DEATHS_SKELETON;
		case SCORPION:
			return STEAM_GSTAT_DEATHS_SCORPION;
		case CREATURE_IMP:
			return STEAM_GSTAT_DEATHS_IMP;
		case GNOME:
			return STEAM_GSTAT_DEATHS_GNOME;
		case DEMON:
			return STEAM_GSTAT_DEATHS_DEMON;
		case SUCCUBUS:
			return STEAM_GSTAT_DEATHS_SUCCUBUS;
		case LICH:
			return STEAM_GSTAT_DEATHS_LICH;
		case MINOTAUR:
			return STEAM_GSTAT_DEATHS_MINOTAUR;
		case DEVIL:
			return STEAM_GSTAT_DEATHS_DEVIL;
		case SHOPKEEPER:
			return STEAM_GSTAT_DEATHS_SHOPKEEPER;
		case KOBOLD:
			return STEAM_GSTAT_DEATHS_KOBOLD;
		case SCARAB:
			return STEAM_GSTAT_DEATHS_SCARAB;
		case CRYSTALGOLEM:
			return STEAM_GSTAT_DEATHS_CRYSTALGOLEM;
		case INCUBUS:
			return STEAM_GSTAT_DEATHS_INCUBUS;
		case VAMPIRE:
			return STEAM_GSTAT_DEATHS_VAMPIRE;
		case SHADOW:
			return STEAM_GSTAT_DEATHS_SHADOW;
		case COCKATRICE:
			return STEAM_GSTAT_DEATHS_COCKATRICE;
		case INSECTOID:
			return STEAM_GSTAT_DEATHS_INSECTOID;
		case GOATMAN:
			return STEAM_GSTAT_DEATHS_GOATMAN;
		case AUTOMATON:
			return STEAM_GSTAT_DEATHS_AUTOMATON;
		case LICH_ICE:
			return STEAM_GSTAT_DEATHS_LICHICE;
		case LICH_FIRE:
			return STEAM_GSTAT_DEATHS_LICHICE;
		case SENTRYBOT:
			return STEAM_GSTAT_DEATHS_SENTRYBOT;
		case SPELLBOT:
			return STEAM_GSTAT_DEATHS_SPELLBOT;
		case GYROBOT:
			return STEAM_GSTAT_DEATHS_GYROBOT;
		case DUMMYBOT:
			return STEAM_GSTAT_DEATHS_DUMMYBOT;
		default:
			return STEAM_GSTAT_INVALID;
	}
	return STEAM_GSTAT_INVALID;
}