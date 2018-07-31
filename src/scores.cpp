/*-------------------------------------------------------------------------------

	BARONY
	File: scores.cpp
	Desc: contains code for handling scores, statistics, and save games

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
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

// definitions
list_t topscores;
list_t topscoresMultiplayer;
int victory = false;
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
bool achievementStatusThankTheTank[MAXPLAYERS] = { false };
std::vector<Uint32> achievementStrobeVec[MAXPLAYERS] = {};
bool achievementStatusStrobe[MAXPLAYERS] = { false };
list_t booksRead;
bool usedClass[NUMCLASSES] = {0};
Uint32 loadingsavegame = 0;
bool achievementBrawlerMode = false;
int savegameCurrentFileIndex = 0;
score_t steamLeaderboardScore;

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
		score->stats->~Stat();
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
		amount += conductGameChallenges[CONDUCT_BLESSED_BOOTS_SPEED] * 100000;
		if ( score->conductGameChallenges[CONDUCT_HARDCORE] == 1
			&& score->conductGameChallenges[CONDUCT_CHEATS_ENABLED] == 0 )
		{
			amount *= 2;
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

	// open file
	if ( (fp = fopen(scoresfilename.c_str(), "wb")) == NULL )
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
		fwrite(&score->stats->appearance, sizeof(Uint32), 1, fp);
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
	if ( (fp = fopen(scoresfilename.c_str(), "rb")) == NULL )
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
		else
		{
			fread(&usedClass[c], sizeof(bool), 1, fp);
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
		fread(&score->stats->name, sizeof(char), 32, fp);
		fread(&score->classnum, sizeof(Sint32), 1, fp);
		fread(&score->dungeonlevel, sizeof(Sint32), 1, fp);
		fread(&score->victory, sizeof(int), 1, fp);
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
			fread(&score->stats->PROFICIENCIES[c], sizeof(Sint32), 1, fp);
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
	int player;
	node_t* node;
	FILE* fp;
	Sint32 c;
	char savefile[64] = "";

	// open file
	if ( !intro )
	{
		messagePlayer(clientnum, language[1121]);
	}

	if ( multiplayer == SINGLE )
	{
		strncpy(savefile, setSaveGameFileName(true, false, saveIndex).c_str(), 63);
	}
	else
	{
		strncpy(savefile, setSaveGameFileName(false, false, saveIndex).c_str(), 63);
	}

	if ( (fp = fopen(savefile, "wb")) == NULL )
	{
		printlog("warning: failed to save '%s'!\n", savefile);
		return 1;
	}

	// write header info
	fprintf(fp, "BARONYSAVEGAME");
	fprintf(fp, VERSION);
	fwrite(&uniqueGameKey, sizeof(Uint32), 1, fp);
	if ( multiplayer > SINGLE && directConnect)
	{
		multiplayer += 2;
		fwrite(&multiplayer, sizeof(Uint32), 1, fp);
		multiplayer -= 2;
	}
	else
	{
		fwrite(&multiplayer, sizeof(Uint32), 1, fp);
	}
	fwrite(&clientnum, sizeof(Uint32), 1, fp);
	fwrite(&mapseed, sizeof(Uint32), 1, fp);
	fwrite(&currentlevel, sizeof(Uint32), 1, fp);
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
		fwrite(&stats[player]->appearance, sizeof(Uint32), 1, fp);
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
		strncpy(savefile, setSaveGameFileName(true, true, saveIndex).c_str(), 63);
	}
	else
	{
		strncpy(savefile, setSaveGameFileName(false, true, saveIndex).c_str(), 63);
	}

	// now we save the follower information
	if ( (fp = fopen(savefile, "wb")) == NULL )
	{
		printlog("warning: failed to save '%s'!\n", savefile);
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

	char savefile[64] = "";
	if ( multiplayer == SINGLE )
	{
		strncpy(savefile, setSaveGameFileName(true, false, saveIndex).c_str(), 63);
	}
	else
	{
		strncpy(savefile, setSaveGameFileName(false, false, saveIndex).c_str(), 63);
	}

	// open file
	if ( (fp = fopen(savefile, "rb")) == NULL )
	{
		printlog("error: failed to load '%s'!\n", savefile);
		return 1;
	}

	// read from file
	char checkstr[64];
	fread(checkstr, sizeof(char), strlen("BARONYSAVEGAME"), fp);
	if ( strncmp(checkstr, "BARONYSAVEGAME", strlen("BARONYSAVEGAME")) )
	{
		printlog("error: '%s' is corrupt!\n", savefile);
		fclose(fp);
		return 1;
	}
	fread(checkstr, sizeof(char), strlen(VERSION), fp);
	int versionNumber = getSavegameVersion(checkstr);
	printlog("loadGame: '%s' version number %d", savefile, versionNumber);
	if ( versionNumber == -1 )
	{
		// if getSavegameVersion returned -1, abort.
		printlog("error: '%s' is corrupt!\n", savefile);
		fclose(fp);
		return 1;
	}
	// read basic header info
	fread(&uniqueGameKey, sizeof(Uint32), 1, fp);
	fread(&mul, sizeof(Uint32), 1, fp);
	fread(&clientnum, sizeof(Uint32), 1, fp);
	fread(&mapseed, sizeof(Uint32), 1, fp);
	fread(&currentlevel, sizeof(Uint32), 1, fp);
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

	// skip through other player data until you get to the correct player
	for ( c = 0; c < player; c++ )
	{
		fseek(fp, sizeof(Uint32), SEEK_CUR);
		fseek(fp, NUMMONSTERS * sizeof(Sint32), SEEK_CUR);
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
		fseek(fp, sizeof(Sint32)*NUMPROFICIENCIES, SEEK_CUR);
		fseek(fp, sizeof(bool)*NUMEFFECTS, SEEK_CUR);
		fseek(fp, sizeof(Sint32)*NUMEFFECTS, SEEK_CUR);

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
	for ( c = 0; c < NUMMONSTERS; c++ )
	{
		fread(&kills[c], sizeof(Sint32), 1, fp);
	}
	fread(&stats[player]->type, sizeof(Monster), 1, fp);
	fread(&stats[player]->sex, sizeof(sex_t), 1, fp);
	fread(&stats[player]->appearance, sizeof(Uint32), 1, fp);
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
		fread(&stats[player]->PROFICIENCIES[c], sizeof(Sint32), 1, fp);
	}
	for ( c = 0; c < NUMEFFECTS; c++ )
	{
		fread(&stats[player]->EFFECTS[c], sizeof(bool), 1, fp);
		fread(&stats[player]->EFFECTS_TIMERS[c], sizeof(Sint32), 1, fp);
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

	char savefile[64] = "";
	if ( multiplayer == SINGLE )
	{
		strncpy(savefile, setSaveGameFileName(true, true, saveIndex).c_str(), 63);
	}
	else
	{
		strncpy(savefile, setSaveGameFileName(false, true, saveIndex).c_str(), 63);
	}

	// open file
	if ( (fp = fopen(savefile, "rb")) == NULL )
	{
		printlog("error: failed to load '%s'!\n", savefile);
		return NULL;
	}

	// read from file
	char checkstr[64];
	fread(checkstr, sizeof(char), strlen("BARONYSAVEGAMEFOLLOWERS"), fp);
	if ( strncmp(checkstr, "BARONYSAVEGAMEFOLLOWERS", strlen("BARONYSAVEGAMEFOLLOWERS")) )
	{
		printlog("error: '%s' is corrupt!\n", savefile);
		fclose(fp);
		return NULL;
	}
	fread(checkstr, sizeof(char), strlen(VERSION), fp);
	int versionNumber = getSavegameVersion(checkstr);
	printlog("loadGameFollowers: '%s' version number %d", savefile, versionNumber);
	if ( versionNumber == -1 )
	{
		// if version number returned is invalid, abort
		printlog("error: '%s' is corrupt!\n", savefile);
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
				fread(&followerStats->PROFICIENCIES[j], sizeof(Sint32), 1, fp);
			}
			for ( j = 0; j < NUMEFFECTS; j++ )
			{
				fread(&followerStats->EFFECTS[j], sizeof(bool), 1, fp);
				fread(&followerStats->EFFECTS_TIMERS[j], sizeof(Sint32), 1, fp);
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
	char savefile[64] = "";
	if ( gametype == SINGLE )
	{
		strncpy(savefile, setSaveGameFileName(true, false, saveIndex).c_str(), 63);
	}
	else
	{
		strncpy(savefile, setSaveGameFileName(false, false, saveIndex).c_str(), 63);
	}
	if (access(savefile, F_OK) != -1)
	{
		printlog("deleting savegame in '%s'...\n", savefile);
		int result = remove(savefile);
		if (result)
		{
			printlog("warning: failed to delete savegame in '%s'!\n", savefile);
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
	if (access(savefile, F_OK) != -1)
	{
		printlog("deleting savegame in '%s'...\n", savefile);
		int result = remove(savefile);
		if (result)
		{
			printlog("warning: failed to delete savegame in '%s'!\n", savefile);
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
	char savefile[64] = "";
	strncpy(savefile, setSaveGameFileName(singleplayer, false, saveIndex).c_str(), 63);

	if ( access(savefile, F_OK ) == -1 )
	{
		return false;
	}
	else
	{
		FILE* fp;
		if ( (fp = fopen(savefile, "rb")) == NULL )
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

	char* tempstr = (char*) calloc(1024, sizeof(char));
	char savefile[64] = "";
	strncpy(savefile, setSaveGameFileName(singleplayer, false, saveIndex).c_str(), 63);

	// open file
	if ( (fp = fopen(savefile, "rb")) == NULL )
	{
		printlog("error: failed to check name in '%s'!\n", savefile);
		free(tempstr);
		return NULL;
	}

	// read from file
	char checkstr[64];
	fread(checkstr, sizeof(char), strlen("BARONYSAVEGAME"), fp);
	if ( strncmp(checkstr, "BARONYSAVEGAME", strlen("BARONYSAVEGAME")) )
	{
		printlog("error: '%s' is corrupt!\n", savefile);
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
		printlog("error: '%s' is corrupt!\n", savefile);
		fclose(fp);
		free(tempstr);
		return nullptr;
	}

	fseek(fp, sizeof(Uint32), SEEK_CUR);
	fread(&mul, sizeof(Uint32), 1, fp);
	fread(&plnum, sizeof(Uint32), 1, fp);
	fseek(fp, sizeof(Uint32), SEEK_CUR);
	fread(&dungeonlevel, sizeof(Uint32), 1, fp);
	fseek(fp,  sizeof(bool), SEEK_CUR);
	if ( versionNumber >= 310 )
	{
		fseek(fp, sizeof(Sint32) * NUM_CONDUCT_CHALLENGES, SEEK_CUR);
		fseek(fp, sizeof(Sint32) * NUM_GAMEPLAY_STATISTICS, SEEK_CUR);
	}
	fseek(fp, sizeof(Uint32)*NUM_HOTBAR_SLOTS, SEEK_CUR);
	fseek(fp, sizeof(Uint32) + sizeof(bool) + sizeof(bool) + sizeof(bool) + sizeof(bool), SEEK_CUR);

	int numspells = 0;
	fread(&numspells, sizeof(Uint32), 1, fp);
	for ( c = 0; c < numspells; c++ )
	{
		fseek(fp, sizeof(Uint32), SEEK_CUR);
	}

	// skip through other player data until you get to the correct player
	for ( c = 0; c < plnum; c++ )
	{
		fseek(fp, sizeof(Uint32), SEEK_CUR);
		fseek(fp, NUMMONSTERS * sizeof(Sint32), SEEK_CUR);
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
		fseek(fp, sizeof(Sint32)*NUMPROFICIENCIES, SEEK_CUR);
		fseek(fp, sizeof(bool)*NUMEFFECTS, SEEK_CUR);
		fseek(fp, sizeof(Sint32)*NUMEFFECTS, SEEK_CUR);

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
	for ( c = 0; c < NUMMONSTERS; c++ )
	{
		fseek(fp, sizeof(Sint32), SEEK_CUR);
	}
	fseek(fp, sizeof(Monster) + sizeof(sex_t) + sizeof(Uint32), SEEK_CUR);
	fread(&name, sizeof(char), 32, fp);
	name[32] = 0;
	fseek(fp, sizeof(Sint32) * 11, SEEK_CUR);
	fread(&level, sizeof(Sint32), 1, fp);

	// assemble string
	char timestamp[128] = "";
#ifdef WINDOWS
	struct _stat result;
	if ( _stat(savefile, &result) == 0 )
	{
		struct tm *tm = localtime(&result.st_mtime);
		if ( tm )
		{
			errno_t err = strftime(timestamp, 127, "%d %b %Y, %H:%M", tm); //day, month, year, time
		}
	}
#else
	struct stat result;
	if ( stat(savefile, &result) == 0 )
	{
		struct tm *tm = localtime(&result.st_mtime);
		if ( tm )
		{
			strftime(timestamp, 127, "%d %b %Y, %H:%M", tm); //day, month, year, time
		}
	}
#endif // WINDOWS
	if ( mul == DIRECTCLIENT || mul == CLIENT )
	{
		// include the player number in the printf.
		snprintf(tempstr, 1024, language[1540 + mul], name, level, playerClassLangEntry(class_), dungeonlevel, plnum, timestamp);
	}
	else
	{
		snprintf(tempstr, 1024, language[1540 + mul], name, level, playerClassLangEntry(class_), dungeonlevel, timestamp);
	}
	// close file
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
	char savefile[64] = "";
	strncpy(savefile, setSaveGameFileName(singleplayer, false, saveIndex).c_str(), 63);

	// open file
	if ( (fp = fopen(savefile, "rb")) == NULL )
	{
		printlog("error: failed to get map seed out of '%s'!\n", savefile);
		return 0;
	}

	// read from file
	char checkstr[64];
	fread(checkstr, sizeof(char), strlen("BARONYSAVEGAME"), fp);
	if ( strncmp(checkstr, "BARONYSAVEGAME", strlen("BARONYSAVEGAME")) )
	{
		printlog("error: '%s' is corrupt!\n", savefile);
		fclose(fp);
		return 0;
	}
	fread(checkstr, sizeof(char), strlen(VERSION), fp);
	int versionNumber = getSavegameVersion(checkstr);
	if ( versionNumber == -1 )
	{
		// if getSavegameVersion returned -1, abort.
		printlog("error: '%s' is corrupt!\n", savefile);
		fclose(fp);
		return 0;
	}

	fread(&gameKey, sizeof(Uint32), 1, fp);

	// close file
	fclose(fp);
	return gameKey;
}

/*-------------------------------------------------------------------------------

	getSaveGameType

	Returns the multiplayer variable stored in the save game

-------------------------------------------------------------------------------*/

int getSaveGameType(bool singleplayer, int saveIndex)
{
	FILE* fp;
	int mul;
	char savefile[64] = "";
	strncpy(savefile, setSaveGameFileName(singleplayer, false, saveIndex).c_str(), 63);

	// open file
	if ( (fp = fopen(savefile, "rb")) == NULL )
	{
		printlog("error: failed to get game type out of '%s'!\n", savefile);
		return 0;
	}

	// read from file
	char checkstr[64];
	fread(checkstr, sizeof(char), strlen("BARONYSAVEGAME"), fp);
	if ( strncmp(checkstr, "BARONYSAVEGAME", strlen("BARONYSAVEGAME")) )
	{
		printlog("error: '%s' is corrupt!\n", savefile);
		fclose(fp);
		return 0;
	}
	fread(checkstr, sizeof(char), strlen(VERSION), fp);
	int versionNumber = getSavegameVersion(checkstr);
	if ( versionNumber == -1 )
	{
		// if getSavegameVersion returned -1, abort.
		printlog("error: '%s' is corrupt!\n", savefile);
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
	char savefile[64] = "";
	strncpy(savefile, setSaveGameFileName(singleplayer, false, saveIndex).c_str(), 63);

	// open file
	if ( (fp = fopen(savefile, "rb")) == NULL )
	{
		printlog("error: failed to get clientnum out of '%s'!\n", savefile);
		return 0;
	}

	// read from file
	char checkstr[64];
	fread(checkstr, sizeof(char), strlen("BARONYSAVEGAME"), fp);
	if ( strncmp(checkstr, "BARONYSAVEGAME", strlen("BARONYSAVEGAME")) )
	{
		printlog("error: '%s' is corrupt!\n", savefile);
		fclose(fp);
		return 0;
	}
	fread(checkstr, sizeof(char), strlen(VERSION), fp);
	int versionNumber = getSavegameVersion(checkstr);
	if ( versionNumber == -1 )
	{
		// if getSavegameVersion returned -1, abort.
		printlog("error: '%s' is corrupt!\n", savefile);
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
	char savefile[64] = "";
	strncpy(savefile, setSaveGameFileName(singleplayer, false, saveIndex).c_str(), 63);

	// open file
	if ( (fp = fopen(savefile, "rb")) == NULL )
	{
		printlog("error: failed to get map seed out of '%s'!\n", savefile);
		return 0;
	}

	// read from file
	char checkstr[64];
	fread(checkstr, sizeof(char), strlen("BARONYSAVEGAME"), fp);
	if ( strncmp(checkstr, "BARONYSAVEGAME", strlen("BARONYSAVEGAME")) )
	{
		printlog("error: '%s' is corrupt!\n", savefile);
		fclose(fp);
		return 0;
	}
	fread(checkstr, sizeof(char), strlen(VERSION), fp);
	int versionNumber = getSavegameVersion(checkstr);
	if ( versionNumber == -1 )
	{
		// if getSavegameVersion returned -1, abort.
		printlog("error: '%s' is corrupt!\n", savefile);
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
	conductGameChallenges[CONDUCT_MODDED] = 0;

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
	}
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
}

void updateGameplayStatisticsInMainLoop()
{
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

void updateAchievementThankTheTank(int player, Entity* target, bool targetKilled)
{
	if ( achievementStatusThankTheTank[player] || multiplayer == CLIENT
		|| player < 0 || player >= MAXPLAYERS )
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

	if ( score->victory == 0 )
	{
		return false;
	}

	//if ( score->conductGameChallenges[CONDUCT_CHEATS_ENABLED] 
	//	|| score->conductGameChallenges[CONDUCT_MODDED] )
	//{
	//	//return false;
	//}

	if ( !score->conductGameChallenges[CONDUCT_MULTIPLAYER] )
	{
		// single player
		if ( !score->conductGameChallenges[CONDUCT_HARDCORE] )
		{
			if ( score->victory == 2 )
			{
				g_SteamLeaderboards->LeaderboardUpload.boardIndex = LEADERBOARD_HELL_TIME;
			}
			else if ( score->victory == 3 )
			{
				g_SteamLeaderboards->LeaderboardUpload.boardIndex = LEADERBOARD_NORMAL_TIME;
			}
			else if ( score->victory == 1 )
			{
				g_SteamLeaderboards->LeaderboardUpload.boardIndex = LEADERBOARD_CLASSIC_TIME;
			}
		}
		else if ( score->conductGameChallenges[CONDUCT_HARDCORE] )
		{
			if ( score->victory == 3 )
			{
				g_SteamLeaderboards->LeaderboardUpload.boardIndex = LEADERBOARD_HARDCORE_TIME;
			}
			else
			{
				g_SteamLeaderboards->LeaderboardUpload.boardIndex = LEADERBOARD_CLASSIC_HARDCORE_TIME;
			}
		}
	}
	else if ( score->conductGameChallenges[CONDUCT_MULTIPLAYER] )
	{
		// multiplayer
		if ( score->victory == 2 )
		{
			g_SteamLeaderboards->LeaderboardUpload.boardIndex = LEADERBOARD_MULTIPLAYER_HELL_TIME;
		}
		else if ( score->victory == 3 )
		{
			g_SteamLeaderboards->LeaderboardUpload.boardIndex = LEADERBOARD_MULTIPLAYER_TIME;
		}
		else if ( score->victory == 1 )
		{
			g_SteamLeaderboards->LeaderboardUpload.boardIndex = LEADERBOARD_MULTIPLAYER_CLASSIC_TIME;
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
	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_RACESEXAPPEARANCECLASS] |= (score->stats->sex) << tagWidth * 1;
	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_RACESEXAPPEARANCECLASS] |= (score->stats->appearance) << tagWidth * 2;
	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_RACESEXAPPEARANCECLASS] |= (score->classnum) << tagWidth * 3;

	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_VICTORYDUNGEONLEVELCONDUCTORIGINAL] |= (score->victory);
	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_VICTORYDUNGEONLEVELCONDUCTORIGINAL] |= (score->dungeonlevel) << tagWidth;
	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_VICTORYDUNGEONLEVELCONDUCTORIGINAL] |= (score->conductPenniless) << tagWidth * 2;
	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_VICTORYDUNGEONLEVELCONDUCTORIGINAL] |= (score->conductFoodless) << tagWidth * 2 + 1;
	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_VICTORYDUNGEONLEVELCONDUCTORIGINAL] |= (score->conductVegetarian) << tagWidth * 2 + 2;
	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_VICTORYDUNGEONLEVELCONDUCTORIGINAL] |= (score->conductIlliterate) << tagWidth * 2 + 3;

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
	// conducts TAG_CONDUCT_4W_1 to TAG_CONDUCT_4W_4 unused.

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
	conductFoodless = (tags[TAG_VICTORYDUNGEONLEVELCONDUCTORIGINAL] >> tagWidth * 2 + 1) & 1;
	conductVegetarian = (tags[TAG_VICTORYDUNGEONLEVELCONDUCTORIGINAL] >> tagWidth * 2 + 2) & 1;
	conductIlliterate = (tags[TAG_VICTORYDUNGEONLEVELCONDUCTORIGINAL] >> tagWidth * 2 + 3) & 1;

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
	// conducts TAG_CONDUCT_4W_1 to TAG_CONDUCT_4W_4 unused.

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
