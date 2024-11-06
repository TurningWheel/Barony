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
#ifdef USE_PLAYFAB
#include "playfab.hpp"
#endif

// definitions
list_t topscores;
list_t topscoresMultiplayer;
int victory = 0;
Uint32 completionTime = 0;
bool conductPenniless = true;
bool conductFoodless = true;
bool conductVegetarian = true;
bool conductIlliterate = true;
Sint32 conductGameChallenges[NUM_CONDUCT_CHALLENGES] = { 0 }; // additional 'conducts' to be stored in here.
Sint32 gameStatistics[NUM_GAMEPLAY_STATISTICS] = { 0 }; // general saved game statistics to be stored in here.
std::vector<std::pair<Uint32, Uint32>> achievementRhythmOfTheKnightVec[MAXPLAYERS] = {};
bool achievementStatusRhythmOfTheKnight[MAXPLAYERS] = { false };
bool achievementRhythmOfTheKnight[MAXPLAYERS] = { false };
std::map<Uint32, Uint32> achievementThankTheTankPair[MAXPLAYERS];
bool achievementStatusBaitAndSwitch[MAXPLAYERS] = { false };
Uint32 achievementBaitAndSwitchTimer[MAXPLAYERS] = { 0 };
std::unordered_set<int> clientLearnedAlchemyIngredients[MAXPLAYERS];
std::vector<std::pair<int, std::pair<int, int>>> clientLearnedAlchemyRecipes[MAXPLAYERS];
std::unordered_set<int> clientLearnedScrollLabels[MAXPLAYERS];
bool achievementStatusThankTheTank[MAXPLAYERS] = { false };
std::vector<Uint32> achievementStrobeVec[MAXPLAYERS] = {};
bool achievementStatusStrobe[MAXPLAYERS] = { false };
bool playerFailedRangedOnlyConduct[MAXPLAYERS] = { false };
list_t booksRead;
bool usedClass[NUMCLASSES] = {0};
bool usedRace[NUMRACES] = { 0 };
Uint32 loadingsavegame = 0;
Uint32 loadinglobbykey = 0;
bool achievementBrawlerMode = false;
bool achievementPenniless = false;
bool achievementRangedMode[MAXPLAYERS] = { 0 };
int savegameCurrentFileIndex = 0;
score_t steamLeaderboardScore;
AchievementObserver achievementObserver;

/*-------------------------------------------------------------------------------

	scoreConstructor

	creates a score_t structure

-------------------------------------------------------------------------------*/

score_t* scoreConstructor(int player)
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
	for ( int c = 0; c < NUMMONSTERS; c++ )
	{
		score->kills[c] = kills[c];
	}
	score->stats->type = stats[player]->type;
	score->stats->sex = stats[player]->sex;
	score->stats->stat_appearance = stats[player]->stat_appearance;
	score->stats->playerRace = stats[player]->playerRace;
	//score->stats->appearance |= stats[player]->playerRace << 8;
	strcpy(score->stats->name, stats[player]->name);
	strcpy(score->stats->obituary, stats[player]->obituary);
	score->stats->killer = stats[player]->killer;
	score->stats->killer_monster = stats[player]->killer_monster;
	score->stats->killer_item = stats[player]->killer_item;
	score->stats->killer_name = stats[player]->killer_name;
	score->totalscore = -1;
	score->victory = victory;
	score->dungeonlevel = currentlevel;
	score->classnum = client_classes[player];
	score->stats->HP = stats[player]->HP;
	score->stats->MAXHP = stats[player]->MAXHP;
	score->stats->MP = stats[player]->MP;
	score->stats->MAXMP = stats[player]->MAXMP;
	score->stats->STR = stats[player]->STR;
	score->stats->DEX = stats[player]->DEX;
	score->stats->CON = stats[player]->CON;
	score->stats->INT = stats[player]->INT;
	score->stats->PER = stats[player]->PER;
	score->stats->CHR = stats[player]->CHR;
	score->stats->EXP = stats[player]->EXP;
	score->stats->LVL = stats[player]->LVL;
	score->stats->GOLD = stats[player]->GOLD;
	score->stats->HUNGER = stats[player]->HUNGER;
	for ( int c = 0; c < NUMPROFICIENCIES; c++ )
	{
		score->stats->setProficiency(c, stats[player]->getProficiency(c));
	}
	for ( int c = 0; c < NUMEFFECTS; c++ )
	{
		score->stats->EFFECTS[c] = stats[player]->EFFECTS[c];
		score->stats->EFFECTS_TIMERS[c] = stats[player]->EFFECTS_TIMERS[c];
	}
	score->stats->leader_uid = 0;
	score->stats->FOLLOWERS.first = NULL;
	score->stats->FOLLOWERS.last = NULL;
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
	list_Copy(&score->stats->inventory, &stats[player]->inventory);
	for ( node = score->stats->inventory.first; node != NULL; node = node->next )
	{
		Item* item = (Item*)node->element;
		item->node = node;
	}
	int c;
	for ( c = 0, node = stats[player]->inventory.first; node != NULL; node = node->next, c++ )
	{
		Item* item = (Item*)node->element;
		if ( stats[player]->helmet == item )
		{
			node_t* node2 = list_Node(&score->stats->inventory, c);
			Item* item2 = (Item*)node2->element;
			score->stats->helmet = item2;
		}
		else if ( stats[player]->breastplate == item )
		{
			node_t* node2 = list_Node(&score->stats->inventory, c);
			Item* item2 = (Item*)node2->element;
			score->stats->breastplate = item2;
		}
		else if ( stats[player]->gloves == item )
		{
			node_t* node2 = list_Node(&score->stats->inventory, c);
			Item* item2 = (Item*)node2->element;
			score->stats->gloves = item2;
		}
		else if ( stats[player]->shoes == item )
		{
			node_t* node2 = list_Node(&score->stats->inventory, c);
			Item* item2 = (Item*)node2->element;
			score->stats->shoes = item2;
		}
		else if ( stats[player]->shield == item )
		{
			node_t* node2 = list_Node(&score->stats->inventory, c);
			Item* item2 = (Item*)node2->element;
			score->stats->shield = item2;
		}
		else if ( stats[player]->weapon == item )
		{
			node_t* node2 = list_Node(&score->stats->inventory, c);
			Item* item2 = (Item*)node2->element;
			score->stats->weapon = item2;
		}
		else if ( stats[player]->cloak == item )
		{
			node_t* node2 = list_Node(&score->stats->inventory, c);
			Item* item2 = (Item*)node2->element;
			score->stats->cloak = item2;
		}
		else if ( stats[player]->amulet == item )
		{
			node_t* node2 = list_Node(&score->stats->inventory, c);
			Item* item2 = (Item*)node2->element;
			score->stats->amulet = item2;
		}
		else if ( stats[player]->ring == item )
		{
			node_t* node2 = list_Node(&score->stats->inventory, c);
			Item* item2 = (Item*)node2->element;
			score->stats->ring = item2;
		}
		else if ( stats[player]->mask == item )
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
	for ( int c = 0; c < NUM_CONDUCT_CHALLENGES; ++c )
	{
		score->conductGameChallenges[c] = conductGameChallenges[c];
	}
	for ( int c = 0; c < NUM_GAMEPLAY_STATISTICS; ++c )
	{
		score->gameStatistics[c] = gameStatistics[c];
	}
	return score;
}

score_t* scoreConstructor(int player, SaveGameInfo& info)
{
	// for online leaderboard loading
	if ( loadGame(player, info) == 0 )
	{
		// store in last player slot to not override current player view
		if ( score_t* score = scoreConstructor(MAXPLAYERS - 1) )
		{
			score->victory = info.hiscore_victory;
			score->stats->killer = (KilledBy)info.hiscore_killed_by;
			score->stats->killer_monster = (Monster)info.hiscore_killed_monster;
			score->stats->killer_item = (ItemType)info.hiscore_killed_item;
			score->totalscore = info.hiscore_totalscore;
			return score;
		}
	}
	return nullptr;
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

int saveScore(int player)
{
    if (currentlevel <= 0) {
        // don't save highscores for level 0...
        return -1;
    }
	score_t* currentscore = scoreConstructor(player);
	list_t* scoresPtr = &topscores;
	if ( conductGameChallenges[CONDUCT_MULTIPLAYER] )
	{
		scoresPtr = &topscoresMultiplayer;
	}

    int c;
    node_t* node;
    Uint32 total = totalScore(currentscore);
	for ( c = 0, node = scoresPtr->first; node != NULL; node = node->next, c++ )
	{
		score_t* score = (score_t*)node->element;
		if ( total > totalScore(score) )
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

	for ( node_t* node = score->stats->inventory.first; node != NULL; node = node->next )
	{
		Item* item = (Item*)node->element;
		amount += items[item->type].value;
	}
	amount += score->stats->GOLD;
	amount += score->stats->EXP;
	amount += score->stats->LVL * 500;

	for ( int c = 0; c < NUMPROFICIENCIES; c++ )
	{
		amount += score->stats->getProficiency(c);
	}
	for ( int c = 0; c < NUMMONSTERS; c++ )
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
	if ( score->victory == 100 )
	{
		amount += 2 * 20000;
	}
	else if ( score->victory == 101 )
	{
		amount += 5 * 20000;
	}
	else if ( score->victory >= 3 )
	{
		amount += 3 * 20000;
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
		amount += score->conductGameChallenges[CONDUCT_BOOTS_SPEED] * 20000;
		amount += score->conductGameChallenges[CONDUCT_BRAWLER] * 20000;
		amount += score->conductGameChallenges[CONDUCT_RANGED_ONLY] * 20000;
		amount += score->conductGameChallenges[CONDUCT_ACCURSED] * 50000;
		amount += score->conductGameChallenges[CONDUCT_BLESSED_BOOTS_SPEED] * 100000;
		if ( score->conductGameChallenges[CONDUCT_HARDCORE] == 1
			&& score->conductGameChallenges[CONDUCT_CHEATS_ENABLED] == 0 )
		{
			amount *= 2;
		}
		if ( score->conductGameChallenges[CONDUCT_KEEPINVENTORY] &&
		    score->conductGameChallenges[CONDUCT_MULTIPLAYER] )
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

void loadScore(score_t* score)
{
	if ( !score ) { return; }
	stats[0]->clearStats();

	for ( int c = 0; c < NUMMONSTERS; c++ )
	{
		kills[c] = score->kills[c];
	}
	stats[0]->type = score->stats->type;
	stats[0]->sex = score->stats->sex;
	stats[0]->stat_appearance = score->stats->stat_appearance;
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

	stats[0]->killer = score->stats->killer;
	stats[0]->killer_monster = score->stats->killer_monster;
	stats[0]->killer_item = score->stats->killer_item;
	stats[0]->killer_name = score->stats->killer_name;

	for ( int c = 0; c < NUMPROFICIENCIES; c++ )
	{
		stats[0]->setProficiency(c, score->stats->getProficiency(c));
	}
	for ( int c = 0; c < NUMEFFECTS; c++ )
	{
		stats[0]->EFFECTS[c] = score->stats->EFFECTS[c];
		stats[0]->EFFECTS_TIMERS[c] = score->stats->EFFECTS_TIMERS[c];
	}
	list_FreeAll(&stats[0]->inventory);
	list_Copy(&stats[0]->inventory, &score->stats->inventory);

	for ( node_t* node = stats[0]->inventory.first; node != NULL; node = node->next )
	{
		Item* item = (Item*)node->element;
		item->node = node;
	}

	int c;
	node_t* node;
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

	for ( int c = 0; c < NUM_CONDUCT_CHALLENGES; ++c )
	{
		conductGameChallenges[c] = score->conductGameChallenges[c];
	}

	for ( int c = 0; c < NUM_GAMEPLAY_STATISTICS; ++c )
	{
		gameStatistics[c] = score->gameStatistics[c];
	}
}

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
	loadScore(score);
}

/*-------------------------------------------------------------------------------

	saveAllScores

	saves all highscores to the scores data file

-------------------------------------------------------------------------------*/

void saveAllScoresJSON(const std::string& scoresfilename)
{
	return;
	char path[PATH_MAX] = "";
	completePath(path, scoresfilename.c_str(), outputdir);

	rapidjson::Document d;
	d.SetObject();
	d.AddMember("version", rapidjson::Value(1), d.GetAllocator());
	d.AddMember("game_version", rapidjson::Value(VERSION), d.GetAllocator());

	int versionNumber = 300;
	char versionStr[4] = "000";
	int i = 0;
	for ( int j = 0; j < strlen(VERSION); ++j )
	{
		if ( VERSION[j] >= '0' && VERSION[j] <= '9' )
		{
			versionStr[i] = VERSION[j]; // copy all integers into versionStr.
			++i;
			if ( i == 3 )
			{
				versionStr[i] = '\0';
				break; // written 3 characters, add termination and break loop.
			}
		}
	}
	versionNumber = atoi(versionStr); // convert from string to int.

	// header info
	{
		rapidjson::Value books_read(rapidjson::kArrayType);
		for ( node_t* node = booksRead.first; node != NULL; node = node->next )
		{
			char* book = (char*)node->element;
			books_read.PushBack(rapidjson::Value(book, d.GetAllocator()), d.GetAllocator());
		}
		d.AddMember("books_read", books_read, d.GetAllocator());
	}

	{
		rapidjson::Value used_class(rapidjson::kArrayType);
		for ( int c = 0; c < NUMCLASSES; c++ )
		{
			used_class.PushBack(rapidjson::Value(usedClass[c]), d.GetAllocator());
		}
		d.AddMember("used_class", used_class, d.GetAllocator());
	}

	{
		rapidjson::Value used_race(rapidjson::kArrayType);
		for ( int c = 0; c < NUMRACES; c++ )
		{
			used_race.PushBack(rapidjson::Value(usedRace[c]), d.GetAllocator());
		}
		d.AddMember("used_race", used_race, d.GetAllocator());
	}

	// score list
	node_t* node = nullptr;
	if ( scoresfilename == "scores.json" )
	{
		node = topscores.first;
	}
	else if ( scoresfilename == "scores_multiplayer.json" )
	{
		node = topscoresMultiplayer.first;
	}
	else
	{
		return;
	}

	rapidjson::Value scores_list(rapidjson::kArrayType);
	for ( ; node != NULL; node = node->next )
	{
		score_t* score = (score_t*)node->element;
		rapidjson::Value entry(rapidjson::kObjectType);

		{
			rapidjson::Value kills_num(rapidjson::kArrayType);
			for ( int c = 0; c < NUMMONSTERS; c++ )
			{
				kills_num.PushBack(kills[c], d.GetAllocator());
			}
			entry.AddMember("kills", kills_num, d.GetAllocator());
		}

		entry.AddMember("completionTime", score->completionTime, d.GetAllocator());
		entry.AddMember("conductPenniless", score->conductPenniless, d.GetAllocator());
		entry.AddMember("conductFoodless", score->conductFoodless, d.GetAllocator());
		entry.AddMember("conductVegetarian", score->conductVegetarian, d.GetAllocator());
		entry.AddMember("conductIlliterate", score->conductIlliterate, d.GetAllocator());
		entry.AddMember("type", score->stats->type, d.GetAllocator());
		entry.AddMember("sex", score->stats->sex, d.GetAllocator());
		entry.AddMember("race", score->stats->playerRace, d.GetAllocator());
		entry.AddMember("appearance", score->stats->stat_appearance, d.GetAllocator());
		entry.AddMember("name", rapidjson::Value(score->stats->name, d.GetAllocator()), d.GetAllocator());

		entry.AddMember("killer_monster", score->stats->killer_monster, d.GetAllocator());
		entry.AddMember("killer_item", score->stats->killer_item, d.GetAllocator());
		entry.AddMember("killer", score->stats->killer, d.GetAllocator());
		entry.AddMember("killer_name", rapidjson::Value(score->stats->killer_name.c_str(), d.GetAllocator()), d.GetAllocator());

		entry.AddMember("classnum", score->classnum, d.GetAllocator());
		entry.AddMember("dungeonlevel", score->dungeonlevel, d.GetAllocator());
		entry.AddMember("victory", score->victory, d.GetAllocator());

		entry.AddMember("HP", score->stats->HP, d.GetAllocator());
		entry.AddMember("MAXHP", score->stats->MAXHP, d.GetAllocator());
		entry.AddMember("MP", score->stats->MP, d.GetAllocator());
		entry.AddMember("MAXMP", score->stats->MAXMP, d.GetAllocator());
		entry.AddMember("STR", score->stats->STR, d.GetAllocator());
		entry.AddMember("DEX", score->stats->DEX, d.GetAllocator());
		entry.AddMember("CON", score->stats->CON, d.GetAllocator());
		entry.AddMember("INT", score->stats->INT, d.GetAllocator());
		entry.AddMember("PER", score->stats->PER, d.GetAllocator());
		entry.AddMember("CHR", score->stats->CHR, d.GetAllocator());
		entry.AddMember("EXP", score->stats->EXP, d.GetAllocator());
		entry.AddMember("LVL", score->stats->LVL, d.GetAllocator());
		entry.AddMember("GOLD", score->stats->GOLD, d.GetAllocator());
		entry.AddMember("HUNGER", score->stats->HUNGER, d.GetAllocator());
		
		{
			rapidjson::Value proficiencies(rapidjson::kArrayType);
			for ( int c = 0; c < NUMPROFICIENCIES; c++ )
			{
				proficiencies.PushBack(score->stats->getProficiency(c), d.GetAllocator());
			}
			entry.AddMember("proficiencies", proficiencies, d.GetAllocator());
		}

		{
			rapidjson::Value effects(rapidjson::kArrayType);
			for ( int c = 0; c < NUMEFFECTS; c++ )
			{
				effects.PushBack(score->stats->EFFECTS[c], d.GetAllocator());
			}
			entry.AddMember("effects", effects, d.GetAllocator());
		}
		{
			rapidjson::Value effects_timers(rapidjson::kArrayType);
			for ( int c = 0; c < NUMEFFECTS; c++ )
			{
				effects_timers.PushBack(score->stats->EFFECTS_TIMERS[c], d.GetAllocator());
			}
			entry.AddMember("effects_timers", effects_timers, d.GetAllocator());
		}


		{
			rapidjson::Value conducts(rapidjson::kArrayType);
			for ( int c = 0; c < NUM_CONDUCT_CHALLENGES; c++ )
			{
				conducts.PushBack(score->conductGameChallenges[c], d.GetAllocator());
			}
			entry.AddMember("conducts", conducts, d.GetAllocator());
		}

		{
			rapidjson::Value statistics(rapidjson::kArrayType);
			for ( int c = 0; c < NUM_GAMEPLAY_STATISTICS; c++ )
			{
				statistics.PushBack(score->gameStatistics[c], d.GetAllocator());
			}
			entry.AddMember("statistics", statistics, d.GetAllocator());
		}

		{
			rapidjson::Value inventory(rapidjson::kArrayType);
			for ( node_t* node2 = score->stats->inventory.first; node2 != NULL; node2 = node2->next )
			{
				Item* item = (Item*)node2->element;

				rapidjson::Value inv_item(rapidjson::kObjectType);
				inv_item.AddMember("type", item->type, d.GetAllocator());
				inv_item.AddMember("status", item->status, d.GetAllocator());
				inv_item.AddMember("beatitude", item->beatitude, d.GetAllocator());
				inv_item.AddMember("count", item->count, d.GetAllocator());
				inv_item.AddMember("appearance", item->appearance, d.GetAllocator());
				inv_item.AddMember("identified", item->identified, d.GetAllocator());

				inventory.PushBack(inv_item, d.GetAllocator());
			}

			entry.AddMember("inventory", inventory, d.GetAllocator());
		}

		{
			rapidjson::Value equipped(rapidjson::kArrayType);
			if ( score->stats->helmet )
			{
				int c = list_Index(score->stats->helmet->node);
				equipped.PushBack(c, d.GetAllocator());
			}
			else
			{
				equipped.PushBack(-1, d.GetAllocator());
			}
			if ( score->stats->breastplate )
			{
				int c = list_Index(score->stats->breastplate->node);
				equipped.PushBack(c, d.GetAllocator());
			}
			else
			{
				equipped.PushBack(-1, d.GetAllocator());
			}
			if ( score->stats->gloves )
			{
				int c = list_Index(score->stats->gloves->node);
				equipped.PushBack(c, d.GetAllocator());
			}
			else
			{
				equipped.PushBack(-1, d.GetAllocator());
			}
			if ( score->stats->shoes )
			{
				int c = list_Index(score->stats->shoes->node);
				equipped.PushBack(c, d.GetAllocator());
			}
			else
			{
				equipped.PushBack(-1, d.GetAllocator());
			}
			if ( score->stats->shield )
			{
				int c = list_Index(score->stats->shield->node);
				equipped.PushBack(c, d.GetAllocator());
			}
			else
			{
				equipped.PushBack(-1, d.GetAllocator());
			}
			if ( score->stats->weapon )
			{
				int c = list_Index(score->stats->weapon->node);
				equipped.PushBack(c, d.GetAllocator());
			}
			else
			{
				equipped.PushBack(-1, d.GetAllocator());
			}
			if ( score->stats->cloak )
			{
				int c = list_Index(score->stats->cloak->node);
				equipped.PushBack(c, d.GetAllocator());
			}
			else
			{
				equipped.PushBack(-1, d.GetAllocator());
			}
			if ( score->stats->amulet )
			{
				int c = list_Index(score->stats->amulet->node);
				equipped.PushBack(c, d.GetAllocator());
			}
			else
			{
				equipped.PushBack(-1, d.GetAllocator());
			}
			if ( score->stats->ring )
			{
				int c = list_Index(score->stats->ring->node);
				equipped.PushBack(c, d.GetAllocator());
			}
			else
			{
				equipped.PushBack(-1, d.GetAllocator());
			}
			if ( score->stats->mask )
			{
				int c = list_Index(score->stats->mask->node);
				equipped.PushBack(c, d.GetAllocator());
			}
			else
			{
				equipped.PushBack(-1, d.GetAllocator());
			}
			entry.AddMember("equipped", equipped, d.GetAllocator());
		}

		scores_list.PushBack(entry, d.GetAllocator());
	}

	d.AddMember("scores_list", scores_list, d.GetAllocator());

	// open file
	File* fp = FileIO::open(path, "wb");
	if ( !fp )
	{
		printlog("[JSON]: Error opening json file %s for write!", path);
		return;
	}

	rapidjson::StringBuffer os;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(os);
	d.Accept(writer);
	fp->write(os.GetString(), sizeof(char), os.GetSize());
	FileIO::close(fp);

	printlog("[JSON]: Successfully wrote json file %s", path);
	return;
}

void saveAllScores(const std::string& scoresfilename)
{
	File* fp;

	char path[PATH_MAX] = "";
	completePath(path, scoresfilename.c_str(), outputdir);

	// open file
	if ( (fp = FileIO::open(path, "wb")) == NULL )
	{
		printlog("error: failed to save '%s!'\n", scoresfilename.c_str());
		return;
	}

	// magic number
	fp->printf("BARONYSCORES");
	fp->printf(VERSION);

	int versionNumber = 300;
	char versionStr[4] = "000";
	int i = 0;
	for ( int j = 0; j < strlen(VERSION); ++j )
	{
		if ( VERSION[j] >= '0' && VERSION[j] <= '9' )
		{
			versionStr[i] = VERSION[j]; // copy all integers into versionStr.
			++i;
			if ( i == 3 )
			{
				versionStr[i] = '\0';
				break; // written 3 characters, add termination and break loop.
			}
		}
	}
	versionNumber = atoi(versionStr); // convert from string to int.

	// header info
	int booksReadNum = list_Size(&booksRead);
	fp->write(&booksReadNum, sizeof(Uint32), 1);
	for ( node_t* node = booksRead.first; node != NULL; node = node->next )
	{
		char* book = (char*)node->element;
		int c = strlen(book);
		fp->write(&c, sizeof(Uint32), 1);
		fp->puts(book);
	}
	for ( int c = 0; c < NUMCLASSES; c++ )
	{
		fp->write(&usedClass[c], sizeof(bool), 1);
	}
	for ( int c = 0; c < NUMRACES; c++ )
	{
		fp->write(&usedRace[c], sizeof(bool), 1);
	}

	// score list
	node_t* node;
	int numScoresInFile;
	if ( scoresfilename.compare(SCORESFILE) == 0 )
	{
		numScoresInFile = list_Size(&topscores);
		node = topscores.first;
	}
	else
	{
		numScoresInFile = list_Size(&topscoresMultiplayer);
		node = topscoresMultiplayer.first;
	}
	fp->write(&numScoresInFile, sizeof(Uint32), 1);

	for (; node != NULL; node = node->next )
	{
		score_t* score = (score_t*)node->element;
		for ( int c = 0; c < NUMMONSTERS; c++ )
		{
			fp->write(&score->kills[c], sizeof(Sint32), 1);
		}
		fp->write(&score->completionTime, sizeof(Uint32), 1);
		fp->write(&score->conductPenniless, sizeof(bool), 1);
		fp->write(&score->conductFoodless, sizeof(bool), 1);
		fp->write(&score->conductVegetarian, sizeof(bool), 1);
		fp->write(&score->conductIlliterate, sizeof(bool), 1);
		fp->write(&score->stats->type, sizeof(Monster), 1);
		fp->write(&score->stats->sex, sizeof(sex_t), 1);
		Uint32 raceAndAppearance = 0;
		raceAndAppearance |= (score->stats->playerRace << 8);
		raceAndAppearance |= (score->stats->stat_appearance);
		fp->write(&raceAndAppearance, sizeof(Uint32), 1);
		fp->write(score->stats->name, sizeof(char), 32);
		if ( versionNumber >= 412 )
		{
			fp->write(&score->stats->killer_monster, sizeof(Uint32), 1);
			fp->write(&score->stats->killer_item, sizeof(Uint32), 1);
			fp->write(&score->stats->killer, sizeof(Uint32), 1);
			char buf[64] = "";
			memset(buf, 0, sizeof(buf));
			snprintf(buf, sizeof(buf), "%s", score->stats->killer_name.c_str());
			fp->write(&buf, sizeof(char), 64);
		}
		else
		{
			score->stats->killer = KilledBy::UNKNOWN;
			score->stats->killer_item = WOODEN_SHIELD;
			score->stats->killer_monster = NOTHING;
			score->stats->killer_name = "";
		}
		fp->write(&score->classnum, sizeof(Sint32), 1);
		fp->write(&score->dungeonlevel, sizeof(Sint32), 1);
		fp->write(&score->victory, sizeof(int), 1);
		fp->write(&score->stats->HP, sizeof(Sint32), 1);
		fp->write(&score->stats->MAXHP, sizeof(Sint32), 1);
		fp->write(&score->stats->MP, sizeof(Sint32), 1);
		fp->write(&score->stats->MAXMP, sizeof(Sint32), 1);
		fp->write(&score->stats->STR, sizeof(Sint32), 1);
		fp->write(&score->stats->DEX, sizeof(Sint32), 1);
		fp->write(&score->stats->CON, sizeof(Sint32), 1);
		fp->write(&score->stats->INT, sizeof(Sint32), 1);
		fp->write(&score->stats->PER, sizeof(Sint32), 1);
		fp->write(&score->stats->CHR, sizeof(Sint32), 1);
		fp->write(&score->stats->EXP, sizeof(Sint32), 1);
		fp->write(&score->stats->LVL, sizeof(Sint32), 1);
		fp->write(&score->stats->GOLD, sizeof(Sint32), 1);
		fp->write(&score->stats->HUNGER, sizeof(Sint32), 1);
		for ( int c = 0; c < NUMPROFICIENCIES; c++ )
		{
			auto val = score->stats->getProficiency(c);
			fp->write(&val, sizeof(Sint32), 1);
		}
		for ( int c = 0; c < NUMEFFECTS; c++ )
		{
			fp->write(&score->stats->EFFECTS[c], sizeof(bool), 1);
			fp->write(&score->stats->EFFECTS_TIMERS[c], sizeof(Sint32), 1);
		}
		for ( int c = 0; c < NUM_CONDUCT_CHALLENGES; ++c )
		{
			fp->write(&score->conductGameChallenges[c], sizeof(Sint32), 1);
		}
		for ( int c = 0; c < NUM_GAMEPLAY_STATISTICS; ++c )
		{
			fp->write(&score->gameStatistics[c], sizeof(Sint32), 1);
		}

		// inventory
		node_t* node2;
		int inventorySize = list_Size(&score->stats->inventory);
		fp->write(&inventorySize, sizeof(ItemType), 1);
		for ( node2 = score->stats->inventory.first; node2 != NULL; node2 = node2->next )
		{
			Item* item = (Item*)node2->element;
			fp->write(&item->type, sizeof(ItemType), 1);
			fp->write(&item->status, sizeof(Status), 1);
			fp->write(&item->beatitude, sizeof(Sint16), 1);
			fp->write(&item->count, sizeof(Sint16), 1);
			fp->write(&item->appearance, sizeof(Uint32), 1);
			fp->write(&item->identified, sizeof(bool), 1);
		}
		if ( score->stats->helmet )
		{
			int c = list_Index(score->stats->helmet->node);
			fp->write(&c, sizeof(ItemType), 1);
		}
		else
		{
			int c = list_Size(&score->stats->inventory);
			fp->write(&c, sizeof(ItemType), 1);
		}
		if ( score->stats->breastplate )
		{
			int c = list_Index(score->stats->breastplate->node);
			fp->write(&c, sizeof(ItemType), 1);
		}
		else
		{
			int c = list_Size(&score->stats->inventory);
			fp->write(&c, sizeof(ItemType), 1);
		}
		if ( score->stats->gloves )
		{
			int c = list_Index(score->stats->gloves->node);
			fp->write(&c, sizeof(ItemType), 1);
		}
		else
		{
			int c = list_Size(&score->stats->inventory);
			fp->write(&c, sizeof(ItemType), 1);
		}
		if ( score->stats->shoes )
		{
			int c = list_Index(score->stats->shoes->node);
			fp->write(&c, sizeof(ItemType), 1);
		}
		else
		{
			int c = list_Size(&score->stats->inventory);
			fp->write(&c, sizeof(ItemType), 1);
		}
		if ( score->stats->shield )
		{
			int c = list_Index(score->stats->shield->node);
			fp->write(&c, sizeof(ItemType), 1);
		}
		else
		{
			int c = list_Size(&score->stats->inventory);
			fp->write(&c, sizeof(ItemType), 1);
		}
		if ( score->stats->weapon )
		{
			int c = list_Index(score->stats->weapon->node);
			fp->write(&c, sizeof(ItemType), 1);
		}
		else
		{
			int c = list_Size(&score->stats->inventory);
			fp->write(&c, sizeof(ItemType), 1);
		}
		if ( score->stats->cloak )
		{
			int c = list_Index(score->stats->cloak->node);
			fp->write(&c, sizeof(ItemType), 1);
		}
		else
		{
			int c = list_Size(&score->stats->inventory);
			fp->write(&c, sizeof(ItemType), 1);
		}
		if ( score->stats->amulet )
		{
			int c = list_Index(score->stats->amulet->node);
			fp->write(&c, sizeof(ItemType), 1);
		}
		else
		{
			int c = list_Size(&score->stats->inventory);
			fp->write(&c, sizeof(ItemType), 1);
		}
		if ( score->stats->ring )
		{
			int c = list_Index(score->stats->ring->node);
			fp->write(&c, sizeof(ItemType), 1);
		}
		else
		{
			int c = list_Size(&score->stats->inventory);
			fp->write(&c, sizeof(ItemType), 1);
		}
		if ( score->stats->mask )
		{
			int c = list_Index(score->stats->mask->node);
			fp->write(&c, sizeof(ItemType), 1);
		}
		else
		{
			int c = list_Size(&score->stats->inventory);
			fp->write(&c, sizeof(ItemType), 1);
		}
	}

	FileIO::close(fp);

	/*if ( scoresfilename == "scores.dat" )
	{
		std::string scoresjson = "scores.json";
		saveAllScoresJSON(scoresjson);
	}
	else if ( scoresfilename == "scores_multiplayer.dat" )
	{
		std::string scoresjson = "scores_multiplayer.json";
		saveAllScoresJSON(scoresjson);
	}*/
}

bool deleteScore(bool multiplayer, int index)
{
    auto node = list_Node(multiplayer ?
        &topscoresMultiplayer : &topscores, index);
    if (node) {
        list_RemoveNode(node);
        return true;
    } else {
        return false;
    }
}

/*-------------------------------------------------------------------------------

	loadAllScores

	loads all highscores from the scores data file

-------------------------------------------------------------------------------*/
int jsonGetInt(rapidjson::Value& d, const char* key)
{
	if ( d.HasMember(key) && d[key].IsInt() )
	{
		return d[key].GetInt();
	}
	return 0;
}

bool jsonGetBool(rapidjson::Value& d, const char* key)
{
	if ( d.HasMember(key) && d[key].IsBool() )
	{
		return d[key].GetBool();
	}
	return false;
}

const char* jsonGetStr(rapidjson::Value& d, const char* key)
{
	if ( d.HasMember(key) && d[key].IsString() )
	{
		return d[key].GetString();
	}
	return "";
}

void loadAllScoresJSON(const std::string& scoresfilename)
{
	return;
	// clear top scores
	if ( scoresfilename == "scores.json" )
	{
		list_FreeAll(&topscores);
	}
	else if ( scoresfilename == "scores_multiplayer.json" )
	{
		list_FreeAll(&topscoresMultiplayer);
	}
	else
	{
		return;
	}

	char path[PATH_MAX] = "";
	completePath(path, scoresfilename.c_str(), outputdir);

	File* fp = nullptr;

	// open file
	if ( (fp = FileIO::open(path, "rb")) == NULL )
	{
		printlog("[JSON]: Error: Could not locate json file %s", path);
		return;
	}

	static char buf[2000000];
	int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
	buf[count] = '\0';
	rapidjson::StringStream is(buf);
	FileIO::close(fp);

	rapidjson::Document d;
	d.ParseStream(is);
	if ( !d.HasMember("version") )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", path);
		return;
	}

	std::string checkStr = jsonGetStr(d, "game_version");
	int versionNumber = 000;
	char versionStr[4] = "000";
	int i = 0;
	for ( int j = 0; j < checkStr.size(); ++j )
	{
		if ( checkStr[j] >= '0' && checkStr[j] <= '9' )
		{
			versionStr[i] = checkStr[j]; // copy all integers into versionStr.
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
		return;
	}

	list_FreeAll(&booksRead);
	if ( d.HasMember("books_read") )
	{
		for ( auto itr = d["books_read"].Begin(); itr != d["books_read"].End(); ++itr )
		{
			std::string bookname = itr->GetString();
			if ( char* book = (char*)malloc(sizeof(char) * (bookname.size() + 1)) )
			{
				memset(book, 0, sizeof(char) * (bookname.size() + 1));
				strcpy(book, bookname.c_str());
				node_t* node = list_AddNodeLast(&booksRead);
				book[bookname.size()] = '\0';
				node->element = book;
				node->size = sizeof(char) * (bookname.size() + 1);
				node->deconstructor = &defaultDeconstructor;
			}
		}
	}

	int c = 0;
	for ( auto itr = d["used_class"].Begin(); itr != d["used_class"].End(); ++itr )
	{
		if ( c >= NUMCLASSES ) { break; }
		usedClass[c] = itr->GetBool();
		++c;
	}

	c = 0;
	for ( auto itr = d["used_race"].Begin(); itr != d["used_race"].End(); ++itr )
	{
		if ( c >= NUMRACES ) { break; }
		usedRace[c] = itr->GetBool();
		++c;
	}

	if ( !d.HasMember("scores_list") )
	{
		return;
	}

	int numscore = 0;
	for ( auto itr = d["scores_list"].Begin(); itr != d["scores_list"].End(); ++itr )
	{
		node_t* node = nullptr;
		if ( scoresfilename == "scores.json" )
		{
			node = list_AddNodeLast(&topscores);
		}
		else if ( scoresfilename == "scores_multiplayer.json" )
		{
			node = list_AddNodeLast(&topscoresMultiplayer);
		}
		else
		{
			break;
		}

		score_t* score = (score_t*)malloc(sizeof(score_t));
		if ( !score )
		{
			printlog("failed to allocate memory for new score!\n");
			exit(1);
		}
		// Stat set to 0 as monster type not needed, values will be overwritten by the savegame data
		score->stats = new Stat(0);
		score->totalscore = -1;
		if ( !score->stats )
		{
			printlog("failed to allocate memory for new stat!\n");
			exit(1);
		}
		node->element = score;
		node->deconstructor = &scoreDeconstructor;
		node->size = sizeof(score_t);

		c = 0;
		for ( auto itr2 = (*itr)["kills"].Begin(); itr2 != (*itr)["kills"].End(); ++itr2 )
		{
			if ( c >= NUMMONSTERS ) { break; }
			score->kills[c] = itr2->GetInt();
			++c;
		}

		score->completionTime = jsonGetInt(*itr, "completionTime");
		score->conductPenniless = jsonGetBool(*itr, "conductPenniless");
		score->conductFoodless = jsonGetBool(*itr, "conductFoodless");
		score->conductVegetarian = jsonGetBool(*itr, "conductVegetarian");
		score->conductIlliterate = jsonGetBool(*itr, "conductIlliterate");
		score->stats->type = (Monster)jsonGetInt(*itr, "type");
		score->stats->sex = (sex_t)jsonGetInt(*itr, "sex");
		score->stats->playerRace = jsonGetInt(*itr, "race");
		score->stats->stat_appearance = (Uint32)jsonGetInt(*itr, "appearance");
		const char* name = jsonGetStr(*itr, "name");
		stringCopy(score->stats->name, name, 32, strlen(name));

		score->stats->killer_monster = (Monster)jsonGetInt(*itr, "killer_monster");
		score->stats->killer_item = (ItemType)jsonGetInt(*itr, "killer_item");
		score->stats->killer = (KilledBy)jsonGetInt(*itr, "killer");
		score->stats->killer_name = jsonGetStr(*itr, "killer_name");

		score->classnum = jsonGetInt(*itr, "classnum");
		score->dungeonlevel = jsonGetInt(*itr, "dungeonlevel");
		score->victory = jsonGetInt(*itr, "victory");
		score->stats->HP = jsonGetInt(*itr, "HP");
		score->stats->MAXHP = jsonGetInt(*itr, "MAXHP");
		score->stats->MP = jsonGetInt(*itr, "MP");
		score->stats->MAXMP = jsonGetInt(*itr, "MAXMP");
		score->stats->STR = jsonGetInt(*itr, "STR");
		score->stats->DEX = jsonGetInt(*itr, "DEX");
		score->stats->CON = jsonGetInt(*itr, "CON");
		score->stats->INT = jsonGetInt(*itr, "INT");
		score->stats->PER = jsonGetInt(*itr, "PER");
		score->stats->CHR = jsonGetInt(*itr, "CHR");
		score->stats->EXP = jsonGetInt(*itr, "EXP");
		score->stats->LVL = jsonGetInt(*itr, "LVL");
		score->stats->GOLD = jsonGetInt(*itr, "GOLD");
		score->stats->HUNGER = jsonGetInt(*itr, "HUNGER");
	}
}

void loadAllScores(const std::string& scoresfilename)
{
	File* fp;
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
	if ( (fp = FileIO::open(path, "rb")) == NULL )
	{
		return;
	}

	// magic number
	char checkstr[64];
	fp->read(checkstr, sizeof(char), strlen("BARONYSCORES"));
	if ( strncmp(checkstr, "BARONYSCORES", strlen("BARONYSCORES")) )
	{
		printlog("error: '%s' is corrupt!\n", scoresfilename.c_str());
		FileIO::close(fp);
		return;
	}

	fp->read(checkstr, sizeof(char), strlen(VERSION));

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
		FileIO::close(fp);
		return;
	}

	// header info
	list_FreeAll(&booksRead);
	fp->read(&c, sizeof(Uint32), 1);
	for ( int i = 0; i < c; i++ )
	{
		// to investigate
		Uint32 booknamelen = 0;
		fp->read(&booknamelen, sizeof(Uint32), 1);

		// old unsafe code using tempstr
		//fp->gets(tempstr, booknamelen + 1);
		//
		//char* book = (char*) malloc(sizeof(char) * (strlen(tempstr) + 1));
		//strcpy(book, tempstr);
		char *book = (char *)malloc(sizeof(char) * (booknamelen + 1));
		fp->gets(book, booknamelen + 1);

		node_t* node = list_AddNodeLast(&booksRead);
		node->element = book;
		//node->size = sizeof(char) * (strlen(tempstr) + 1);
		node->size = sizeof(char) * (booknamelen + 1);
		node->deconstructor = &defaultDeconstructor;
	}
	for ( int c = 0; c < NUMCLASSES; c++ )
	{
		if ( versionNumber < 300 )
		{
			if ( c < 10 )
			{
				fp->read(&usedClass[c], sizeof(bool), 1);
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
				fp->read(&usedClass[c], sizeof(bool), 1);
			}
			else
			{
				usedClass[c] = false;
			}
		}
		else
		{
			fp->read(&usedClass[c], sizeof(bool), 1);
		}
	}

	for ( int c = 0; c < NUMRACES; c++ )
	{
		if ( versionNumber <= 325 )
		{
			// don't read race info.
			usedRace[c] = false;
		}
		else
		{
			fp->read(&usedRace[c], sizeof(bool), 1);
		}
	}

	// read scores
	Uint32 numscores = 0;
	fp->read(&numscores, sizeof(Uint32), 1);
	for ( int i = 0; i < numscores; i++ )
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
		score->totalscore = -1;
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
			for ( int c = 0; c < NUMMONSTERS; c++ )
			{
				if ( c < 21 )
				{
					fp->read(&score->kills[c], sizeof(Sint32), 1);
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
			for ( int c = 0; c < NUMMONSTERS; c++ )
			{
				if ( c < 33 )
				{
					fp->read(&score->kills[c], sizeof(Sint32), 1);
				}
				else
				{
					score->kills[c] = 0;
				}
			}
		}
		else if ( versionNumber < 422 )
		{
			// legacy nummonsters
			for ( int c = 0; c < NUMMONSTERS; c++ )
			{
				if ( c < 37 )
				{
					fp->read(&score->kills[c], sizeof(Sint32), 1);
				}
				else
				{
					score->kills[c] = 0;
				}
			}
		}
		else
		{
			for ( int c = 0; c < NUMMONSTERS; c++ )
			{
				fp->read(&score->kills[c], sizeof(Sint32), 1);
			}
		}
		fp->read(&score->completionTime, sizeof(Uint32), 1);
		fp->read(&score->conductPenniless, sizeof(bool), 1);
		fp->read(&score->conductFoodless, sizeof(bool), 1);
		fp->read(&score->conductVegetarian, sizeof(bool), 1);
		fp->read(&score->conductIlliterate, sizeof(bool), 1);
		fp->read(&score->stats->type, sizeof(Monster), 1);
		fp->read(&score->stats->sex, sizeof(sex_t), 1);
		fp->read(&score->stats->stat_appearance, sizeof(Uint32), 1);
		if ( versionNumber >= 323 )
		{
			score->stats->playerRace = ((score->stats->stat_appearance & 0xFF00) >> 8);
			score->stats->stat_appearance = (score->stats->stat_appearance & 0xFF);
		}
		fp->read(&score->stats->name, sizeof(char), 32);
		if ( versionNumber >= 412 )
		{
			fp->read(&score->stats->killer_monster, sizeof(Uint32), 1);
			fp->read(&score->stats->killer_item, sizeof(Uint32), 1);
			fp->read(&score->stats->killer, sizeof(Uint32), 1);
			char buf[64] = "";
			memset(buf, 0, sizeof(buf));
			fp->read(&buf, sizeof(char), 64);
			score->stats->killer_name = buf;
		}
		else
		{
			score->stats->killer = KilledBy::UNKNOWN;
			score->stats->killer_item = WOODEN_SHIELD;
			score->stats->killer_monster = NOTHING;
			score->stats->killer_name = "";
		}
		fp->read(&score->classnum, sizeof(Sint32), 1);
		fp->read(&score->dungeonlevel, sizeof(Sint32), 1);
		fp->read(&score->victory, sizeof(int), 1);
		fp->read(&score->stats->HP, sizeof(Sint32), 1);
		fp->read(&score->stats->MAXHP, sizeof(Sint32), 1);
		fp->read(&score->stats->MP, sizeof(Sint32), 1);
		fp->read(&score->stats->MAXMP, sizeof(Sint32), 1);
		fp->read(&score->stats->STR, sizeof(Sint32), 1);
		fp->read(&score->stats->DEX, sizeof(Sint32), 1);
		fp->read(&score->stats->CON, sizeof(Sint32), 1);
		fp->read(&score->stats->INT, sizeof(Sint32), 1);
		fp->read(&score->stats->PER, sizeof(Sint32), 1);
		fp->read(&score->stats->CHR, sizeof(Sint32), 1);
		fp->read(&score->stats->EXP, sizeof(Sint32), 1);
		fp->read(&score->stats->LVL, sizeof(Sint32), 1);
		fp->read(&score->stats->GOLD, sizeof(Sint32), 1);
		fp->read(&score->stats->HUNGER, sizeof(Sint32), 1);
		for ( int c = 0; c < NUMPROFICIENCIES; c++ )
		{
			if ( versionNumber < 323 && c >= PRO_UNARMED )
			{
				score->stats->setProficiency(c, 0);
			}
			else
			{
				Sint32 val = 0;
				fp->read(&val, sizeof(Sint32), 1);
				score->stats->setProficiency(c, val);
			}
		}
		if ( versionNumber < 300 )
		{
			// legacy effects
			for ( int c = 0; c < NUMEFFECTS; c++ )
			{
				if ( c < 16 )
				{
					fp->read(&score->stats->EFFECTS[c], sizeof(bool), 1);
					fp->read(&score->stats->EFFECTS_TIMERS[c], sizeof(Sint32), 1);
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
			for ( int c = 0; c < NUMEFFECTS; c++ )
			{
				if ( c < 19 )
				{
					fp->read(&score->stats->EFFECTS[c], sizeof(bool), 1);
					fp->read(&score->stats->EFFECTS_TIMERS[c], sizeof(Sint32), 1);
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
			for ( int c = 0; c < NUMEFFECTS; c++ )
			{
				if ( c < 32 )
				{
					fp->read(&score->stats->EFFECTS[c], sizeof(bool), 1);
					fp->read(&score->stats->EFFECTS_TIMERS[c], sizeof(Sint32), 1);
				}
				else
				{
					score->stats->EFFECTS[c] = false;
					score->stats->EFFECTS_TIMERS[c] = 0;
				}
			}
		}
		else if ( versionNumber <= 411 )
		{
			for ( int c = 0; c < NUMEFFECTS; c++ )
			{
				if ( c < 40 )
				{
					fp->read(&score->stats->EFFECTS[c], sizeof(bool), 1);
					fp->read(&score->stats->EFFECTS_TIMERS[c], sizeof(Sint32), 1);
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
			for ( int c = 0; c < NUMEFFECTS; c++ )
			{
				fp->read(&score->stats->EFFECTS[c], sizeof(bool), 1);
				fp->read(&score->stats->EFFECTS_TIMERS[c], sizeof(Sint32), 1);
			}
		}

		if ( versionNumber >= 310 )
		{
			for ( int c = 0; c < NUM_CONDUCT_CHALLENGES; ++c )
			{
				fp->read(&score->conductGameChallenges[c], sizeof(Sint32), 1);
			}
			for ( int c = 0; c < NUM_GAMEPLAY_STATISTICS; ++c )
			{
				fp->read(&score->gameStatistics[c], sizeof(Sint32), 1);
			}
		}
		else
		{
			for ( int c = 0; c < NUM_CONDUCT_CHALLENGES; ++c )
			{
				score->conductGameChallenges[c] = 0;
			}
		}
		score->stats->leader_uid = 0;
		score->stats->FOLLOWERS.first = NULL;
		score->stats->FOLLOWERS.last = NULL;

		// inventory
		int numitems = 0;
		fp->read(&numitems, sizeof(Uint32), 1);
		score->stats->inventory.first = NULL;
		score->stats->inventory.last = NULL;
		for ( int c = 0; c < numitems; c++ )
		{
			ItemType type;
			Status status;
			Sint16 beatitude;
			Sint16 count;
			Uint32 appearance;
			bool identified;
			fp->read(&type, sizeof(ItemType), 1);
			fp->read(&status, sizeof(Status), 1);
			fp->read(&beatitude, sizeof(Sint16), 1);
			fp->read(&count, sizeof(Sint16), 1);
			fp->read(&appearance, sizeof(Uint32), 1);
			fp->read(&identified, sizeof(bool), 1);
			newItem(type, status, beatitude, count, appearance, identified, &score->stats->inventory);
		}
		fp->read(&c, sizeof(Uint32), 1);
		node = list_Node(&score->stats->inventory, c);
		if ( node )
		{
			score->stats->helmet = (Item*)node->element;
		}
		else
		{
			score->stats->helmet = NULL;
		}
		fp->read(&c, sizeof(Uint32), 1);
		node = list_Node(&score->stats->inventory, c);
		if ( node )
		{
			score->stats->breastplate = (Item*)node->element;
		}
		else
		{
			score->stats->breastplate = NULL;
		}
		fp->read(&c, sizeof(Uint32), 1);
		node = list_Node(&score->stats->inventory, c);
		if ( node )
		{
			score->stats->gloves = (Item*)node->element;
		}
		else
		{
			score->stats->gloves = NULL;
		}
		fp->read(&c, sizeof(Uint32), 1);
		node = list_Node(&score->stats->inventory, c);
		if ( node )
		{
			score->stats->shoes = (Item*)node->element;
		}
		else
		{
			score->stats->shoes = NULL;
		}
		fp->read(&c, sizeof(Uint32), 1);
		node = list_Node(&score->stats->inventory, c);
		if ( node )
		{
			score->stats->shield = (Item*)node->element;
		}
		else
		{
			score->stats->shield = NULL;
		}
		fp->read(&c, sizeof(Uint32), 1);
		node = list_Node(&score->stats->inventory, c);
		if ( node )
		{
			score->stats->weapon = (Item*)node->element;
		}
		else
		{
			score->stats->weapon = NULL;
		}
		fp->read(&c, sizeof(Uint32), 1);
		node = list_Node(&score->stats->inventory, c);
		if ( node )
		{
			score->stats->cloak = (Item*)node->element;
		}
		else
		{
			score->stats->cloak = NULL;
		}
		fp->read(&c, sizeof(Uint32), 1);
		node = list_Node(&score->stats->inventory, c);
		if ( node )
		{
			score->stats->amulet = (Item*)node->element;
		}
		else
		{
			score->stats->amulet = NULL;
		}
		fp->read(&c, sizeof(Uint32), 1);
		node = list_Node(&score->stats->inventory, c);
		if ( node )
		{
			score->stats->ring = (Item*)node->element;
		}
		else
		{
			score->stats->ring = NULL;
		}
		fp->read(&c, sizeof(Uint32), 1);
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

	FileIO::close(fp);
}

/*-------------------------------------------------------------------------------

	saveGameOld

	Saves the player character as they were at the start of the
	last level

-------------------------------------------------------------------------------*/

int saveGameOld(int saveIndex)
{
	if ( gameModeManager.getMode() != GameModeManager_t::GAME_MODE_DEFAULT )
	{
		return 1;
	}

	File* fp;
	char savefile[PATH_MAX] = "";
	char path[PATH_MAX] = "";

	// open file
	if ( !intro )
	{
		messagePlayer(clientnum, MESSAGE_MISC, Language::get(1121));
	}

	if ( multiplayer == SINGLE )
	{
		strncpy(savefile, setSaveGameFileName(true, SaveFileType::MAIN, saveIndex).c_str(), PATH_MAX - 1);
	}
	else
	{
		strncpy(savefile, setSaveGameFileName(false, SaveFileType::MAIN, saveIndex).c_str(), PATH_MAX - 1);
	}
	completePath(path, savefile, outputdir);

	if ( (fp = FileIO::open(path, "wb")) == NULL )
	{
		printlog("warning: failed to save '%s'!\n", path);
		return 1;
	}

	// write header info
	fp->printf("BARONYSAVEGAME");
	fp->printf(VERSION);
	fp->write(&uniqueGameKey, sizeof(Uint32), 1);

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

	Sint16 players_connected = 0;
	for (int c = 0; c < MAXPLAYERS; ++c) {
	    if (!client_disconnected[c]) {
            players_connected |= 1 << c;
        }
	}
	fp->write(&players_connected, sizeof(Sint16), 1);

	Sint16 mul = 0;
	if ( multiplayer == SINGLE ) {
	    if (splitscreen) {
	        mul = SPLITSCREEN;
	    } else {
		    mul = SINGLE;
		}
	} else {
	    if (multiplayer == SERVER && directConnect) {
	        mul = DIRECTSERVER;
	    }
	    else if (multiplayer == SERVER && LobbyHandler.hostingType == LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY) {
			mul = SERVERCROSSPLAY;
		}
	    else if (multiplayer == SERVER || multiplayer == CLIENT) {
	        mul = directConnect ? multiplayer + 2 : multiplayer;
	    }
		else {
		    assert(0 && "Unknown game save type!");
		}
	}
	fp->write(&mul, sizeof(Sint16), 1);

	fp->write(&clientnum, sizeof(Uint32), 1);
	fp->write(&mapseed, sizeof(Uint32), 1);
	fp->write(&writeCurrentLevel, sizeof(Uint32), 1);
	fp->write(&secretlevel, sizeof(bool), 1);
	fp->write(&completionTime, sizeof(Uint32), 1);
	fp->write(&conductPenniless, sizeof(bool), 1);
	fp->write(&conductFoodless, sizeof(bool), 1);
	fp->write(&conductVegetarian, sizeof(bool), 1);
	fp->write(&conductIlliterate, sizeof(bool), 1);
	for ( int c = 0; c < NUM_CONDUCT_CHALLENGES; ++c )
	{
		fp->write(&conductGameChallenges[c], sizeof(Sint32), 1);
	}
	for ( int c = 0; c < NUM_GAMEPLAY_STATISTICS; ++c )
	{
		fp->write(&gameStatistics[c], sizeof(Sint32), 1);
	}
	fp->write(&svFlags, sizeof(Uint32), 1);

    if (splitscreen)
    {
	    for ( int player = 0; player < MAXPLAYERS; player++ )
	    {
	        // write hotbar items
	        for ( auto& hotbarSlot : players[player]->hotbar.slots() )
	        {
		        int index = list_Size(&stats[player]->inventory);
		        Item* item = uidToItem(hotbarSlot.item);
		        if ( item )
		        {
			        index = list_Index(item->node);
		        }
		        fp->write(&index, sizeof(Uint32), 1);
	        }

	        // write spells
	        Uint32 numspells = list_Size(&players[player]->magic.spellList);
	        fp->write(&numspells, sizeof(Uint32), 1);
	        for ( node_t* node = players[player]->magic.spellList.first; node != NULL; node = node->next )
	        {
		        spell_t* spell = (spell_t*)node->element;
		        fp->write(&spell->ID, sizeof(Uint32), 1);
	        }

			// write alchemy recipes
			Uint32 numrecipes = clientLearnedAlchemyRecipes[player].size();
			fp->write(&numrecipes, sizeof(Uint32), 1);
			for ( auto& entry : clientLearnedAlchemyRecipes[player] )
			{
				fp->write(&entry.first, sizeof(Sint32), 1);
				fp->write(&entry.second.first, sizeof(Sint32), 1);
				fp->write(&entry.second.second, sizeof(Sint32), 1);
			}

			// write scrolls known
			Uint32 numscrolls = clientLearnedScrollLabels[player].size();
			fp->write(&numscrolls, sizeof(Uint32), 1);
			for ( auto& entry : clientLearnedScrollLabels[player] )
			{
				fp->write(&entry, sizeof(Sint32), 1);
			}
	    }
    }
    else
    {
	    // write hotbar items
	    for ( auto& hotbarSlot : players[clientnum]->hotbar.slots() )
	    {
		    int index = list_Size(&stats[clientnum]->inventory);
		    Item* item = uidToItem(hotbarSlot.item);
		    if ( item )
		    {
			    index = list_Index(item->node);
		    }
		    fp->write(&index, sizeof(Uint32), 1);
	    }

	    // write spells
	    Uint32 numspells = list_Size(&players[clientnum]->magic.spellList);
	    fp->write(&numspells, sizeof(Uint32), 1);
	    for ( node_t* node = players[clientnum]->magic.spellList.first; node != NULL; node = node->next )
	    {
		    spell_t* spell = (spell_t*)node->element;
		    fp->write(&spell->ID, sizeof(Uint32), 1);
	    }

		// write alchemy recipes
		Uint32 numrecipes = clientLearnedAlchemyRecipes[clientnum].size();
		fp->write(&numrecipes, sizeof(Uint32), 1);
		for ( auto& entry : clientLearnedAlchemyRecipes[clientnum] )
		{
			fp->write(&entry.first, sizeof(Sint32), 1);
			fp->write(&entry.second.first, sizeof(Sint32), 1);
			fp->write(&entry.second.second, sizeof(Sint32), 1);
		}

		// write scrolls known
		Uint32 numscrolls = clientLearnedScrollLabels[clientnum].size();
		fp->write(&numscrolls, sizeof(Uint32), 1);
		for ( auto& entry : clientLearnedScrollLabels[clientnum] )
		{
			fp->write(&entry, sizeof(Sint32), 1);
		}
    }

	// player data
	for ( int player = 0; player < MAXPLAYERS; player++ )
	{
		fp->write(&client_classes[player], sizeof(Uint32), 1);
		for ( int c = 0; c < NUMMONSTERS; c++ )
		{
			fp->write(&kills[c], sizeof(Sint32), 1);
		}
		fp->write(&stats[player]->type, sizeof(Monster), 1);
		fp->write(&stats[player]->sex, sizeof(sex_t), 1);
		Uint32 raceAndAppearance = 0;
		raceAndAppearance |= (stats[player]->playerRace << 8);
		raceAndAppearance |= (stats[player]->stat_appearance);
		fp->write(&raceAndAppearance, sizeof(Uint32), 1);
		fp->write(stats[player]->name, sizeof(char), 32);
		fp->write(&stats[player]->HP, sizeof(Sint32), 1);
		fp->write(&stats[player]->MAXHP, sizeof(Sint32), 1);
		fp->write(&stats[player]->MP, sizeof(Sint32), 1);
		fp->write(&stats[player]->MAXMP, sizeof(Sint32), 1);
		fp->write(&stats[player]->STR, sizeof(Sint32), 1);
		fp->write(&stats[player]->DEX, sizeof(Sint32), 1);
		fp->write(&stats[player]->CON, sizeof(Sint32), 1);
		fp->write(&stats[player]->INT, sizeof(Sint32), 1);
		fp->write(&stats[player]->PER, sizeof(Sint32), 1);
		fp->write(&stats[player]->CHR, sizeof(Sint32), 1);
		fp->write(&stats[player]->EXP, sizeof(Sint32), 1);
		fp->write(&stats[player]->LVL, sizeof(Sint32), 1);
		fp->write(&stats[player]->GOLD, sizeof(Sint32), 1);
		fp->write(&stats[player]->HUNGER, sizeof(Sint32), 1);
		for ( int c = 0; c < NUMPROFICIENCIES; c++ )
		{
			Sint32 val = stats[player]->getProficiency(c);
			fp->write(&val, sizeof(Sint32), 1);
		}
		for ( int c = 0; c < NUMEFFECTS; c++ )
		{
			fp->write(&stats[player]->EFFECTS[c], sizeof(bool), 1);
			fp->write(&stats[player]->EFFECTS_TIMERS[c], sizeof(Sint32), 1);
		}
		for ( int c = 0; c < 32; c++ )
		{
			fp->write(&stats[player]->MISC_FLAGS[c], sizeof(Sint32), 1);
		}

		// inventory
		if ( player == clientnum || splitscreen )
		{
			int c = list_Size(&stats[player]->inventory);
			fp->write(&c, sizeof(Uint32), 1);
			for ( node_t* node = stats[player]->inventory.first; node != NULL; node = node->next )
			{
				Item* item = (Item*)node->element;
				fp->write(&item->type, sizeof(ItemType), 1);
				fp->write(&item->status, sizeof(Status), 1);
				fp->write(&item->beatitude, sizeof(Sint16), 1);
				fp->write(&item->count, sizeof(Sint16), 1);
				fp->write(&item->appearance, sizeof(Uint32), 1);
				fp->write(&item->identified, sizeof(bool), 1);
				fp->write(&item->x, sizeof(Sint32), 1);
				fp->write(&item->y, sizeof(Sint32), 1);
			}
			if ( stats[player]->helmet )
			{
				c = list_Index(stats[player]->helmet->node);
				fp->write(&c, sizeof(Uint32), 1);
			}
			else
			{
				c = list_Size(&stats[player]->inventory);
				fp->write(&c, sizeof(Uint32), 1);
			}
			if ( stats[player]->breastplate )
			{
				c = list_Index(stats[player]->breastplate->node);
				fp->write(&c, sizeof(Uint32), 1);
			}
			else
			{
				c = list_Size(&stats[player]->inventory);
				fp->write(&c, sizeof(Uint32), 1);
			}
			if ( stats[player]->gloves )
			{
				c = list_Index(stats[player]->gloves->node);
				fp->write(&c, sizeof(Uint32), 1);
			}
			else
			{
				c = list_Size(&stats[player]->inventory);
				fp->write(&c, sizeof(Uint32), 1);
			}
			if ( stats[player]->shoes )
			{
				c = list_Index(stats[player]->shoes->node);
				fp->write(&c, sizeof(Uint32), 1);
			}
			else
			{
				c = list_Size(&stats[player]->inventory);
				fp->write(&c, sizeof(Uint32), 1);
			}
			if ( stats[player]->shield )
			{
				c = list_Index(stats[player]->shield->node);
				fp->write(&c, sizeof(Uint32), 1);
			}
			else
			{
				c = list_Size(&stats[player]->inventory);
				fp->write(&c, sizeof(Uint32), 1);
			}
			if ( stats[player]->weapon )
			{
				c = list_Index(stats[player]->weapon->node);
				fp->write(&c, sizeof(Uint32), 1);
			}
			else
			{
				c = list_Size(&stats[player]->inventory);
				fp->write(&c, sizeof(Uint32), 1);
			}
			if ( stats[player]->cloak )
			{
				c = list_Index(stats[player]->cloak->node);
				fp->write(&c, sizeof(Uint32), 1);
			}
			else
			{
				c = list_Size(&stats[player]->inventory);
				fp->write(&c, sizeof(Uint32), 1);
			}
			if ( stats[player]->amulet )
			{
				c = list_Index(stats[player]->amulet->node);
				fp->write(&c, sizeof(Uint32), 1);
			}
			else
			{
				c = list_Size(&stats[player]->inventory);
				fp->write(&c, sizeof(Uint32), 1);
			}
			if ( stats[player]->ring )
			{
				c = list_Index(stats[player]->ring->node);
				fp->write(&c, sizeof(Uint32), 1);
			}
			else
			{
				c = list_Size(&stats[player]->inventory);
				fp->write(&c, sizeof(Uint32), 1);
			}
			if ( stats[player]->mask )
			{
				c = list_Index(stats[player]->mask->node);
				fp->write(&c, sizeof(Uint32), 1);
			}
			else
			{
				c = list_Size(&stats[player]->inventory);
				fp->write(&c, sizeof(Uint32), 1);
			}
		}
		else
		{
			if ( multiplayer == SERVER )
			{
				if ( stats[player]->helmet )
				{
					Item* item = stats[player]->helmet;
					fp->write(&item->type, sizeof(ItemType), 1);
					fp->write(&item->status, sizeof(Status), 1);
					fp->write(&item->beatitude, sizeof(Sint16), 1);
					fp->write(&item->count, sizeof(Sint16), 1);
					fp->write(&item->appearance, sizeof(Uint32), 1);
					fp->write(&item->identified, sizeof(bool), 1);
				}
				else
				{
					int c = NUMITEMS;
					fp->write(&c, sizeof(ItemType), 1);
				}
				if ( stats[player]->breastplate )
				{
					Item* item = stats[player]->breastplate;
					fp->write(&item->type, sizeof(ItemType), 1);
					fp->write(&item->status, sizeof(Status), 1);
					fp->write(&item->beatitude, sizeof(Sint16), 1);
					fp->write(&item->count, sizeof(Sint16), 1);
					fp->write(&item->appearance, sizeof(Uint32), 1);
					fp->write(&item->identified, sizeof(bool), 1);
				}
				else
				{
					int c = NUMITEMS;
					fp->write(&c, sizeof(ItemType), 1);
				}
				if ( stats[player]->gloves )
				{
					Item* item = stats[player]->gloves;
					fp->write(&item->type, sizeof(ItemType), 1);
					fp->write(&item->status, sizeof(Status), 1);
					fp->write(&item->beatitude, sizeof(Sint16), 1);
					fp->write(&item->count, sizeof(Sint16), 1);
					fp->write(&item->appearance, sizeof(Uint32), 1);
					fp->write(&item->identified, sizeof(bool), 1);
				}
				else
				{
					int c = NUMITEMS;
					fp->write(&c, sizeof(ItemType), 1);
				}
				if ( stats[player]->shoes )
				{
					Item* item = stats[player]->shoes;
					fp->write(&item->type, sizeof(ItemType), 1);
					fp->write(&item->status, sizeof(Status), 1);
					fp->write(&item->beatitude, sizeof(Sint16), 1);
					fp->write(&item->count, sizeof(Sint16), 1);
					fp->write(&item->appearance, sizeof(Uint32), 1);
					fp->write(&item->identified, sizeof(bool), 1);
				}
				else
				{
					int c = NUMITEMS;
					fp->write(&c, sizeof(ItemType), 1);
				}
				if ( stats[player]->shield )
				{
					Item* item = stats[player]->shield;
					fp->write(&item->type, sizeof(ItemType), 1);
					fp->write(&item->status, sizeof(Status), 1);
					fp->write(&item->beatitude, sizeof(Sint16), 1);
					fp->write(&item->count, sizeof(Sint16), 1);
					fp->write(&item->appearance, sizeof(Uint32), 1);
					fp->write(&item->identified, sizeof(bool), 1);
				}
				else
				{
					int c = NUMITEMS;
					fp->write(&c, sizeof(ItemType), 1);
				}
				if ( stats[player]->weapon )
				{
					Item* item = stats[player]->weapon;
					fp->write(&item->type, sizeof(ItemType), 1);
					fp->write(&item->status, sizeof(Status), 1);
					fp->write(&item->beatitude, sizeof(Sint16), 1);
					fp->write(&item->count, sizeof(Sint16), 1);
					fp->write(&item->appearance, sizeof(Uint32), 1);
					fp->write(&item->identified, sizeof(bool), 1);
				}
				else
				{
					int c = NUMITEMS;
					fp->write(&c, sizeof(ItemType), 1);
				}
				if ( stats[player]->cloak )
				{
					Item* item = stats[player]->cloak;
					fp->write(&item->type, sizeof(ItemType), 1);
					fp->write(&item->status, sizeof(Status), 1);
					fp->write(&item->beatitude, sizeof(Sint16), 1);
					fp->write(&item->count, sizeof(Sint16), 1);
					fp->write(&item->appearance, sizeof(Uint32), 1);
					fp->write(&item->identified, sizeof(bool), 1);
				}
				else
				{
					int c = NUMITEMS;
					fp->write(&c, sizeof(ItemType), 1);
				}
				if ( stats[player]->amulet )
				{
					Item* item = stats[player]->amulet;
					fp->write(&item->type, sizeof(ItemType), 1);
					fp->write(&item->status, sizeof(Status), 1);
					fp->write(&item->beatitude, sizeof(Sint16), 1);
					fp->write(&item->count, sizeof(Sint16), 1);
					fp->write(&item->appearance, sizeof(Uint32), 1);
					fp->write(&item->identified, sizeof(bool), 1);
				}
				else
				{
					int c = NUMITEMS;
					fp->write(&c, sizeof(ItemType), 1);
				}
				if ( stats[player]->ring )
				{
					Item* item = stats[player]->ring;
					fp->write(&item->type, sizeof(ItemType), 1);
					fp->write(&item->status, sizeof(Status), 1);
					fp->write(&item->beatitude, sizeof(Sint16), 1);
					fp->write(&item->count, sizeof(Sint16), 1);
					fp->write(&item->appearance, sizeof(Uint32), 1);
					fp->write(&item->identified, sizeof(bool), 1);
				}
				else
				{
					int c = NUMITEMS;
					fp->write(&c, sizeof(ItemType), 1);
				}
				if ( stats[player]->mask )
				{
					Item* item = stats[player]->mask;
					fp->write(&item->type, sizeof(ItemType), 1);
					fp->write(&item->status, sizeof(Status), 1);
					fp->write(&item->beatitude, sizeof(Sint16), 1);
					fp->write(&item->count, sizeof(Sint16), 1);
					fp->write(&item->appearance, sizeof(Uint32), 1);
					fp->write(&item->identified, sizeof(bool), 1);
				}
				else
				{
					int c = NUMITEMS;
					fp->write(&c, sizeof(ItemType), 1);
				}
			}
			else
			{
				int c = NUMITEMS;
				fp->write(&c, sizeof(ItemType), 1);
			}
		}
	}
	FileIO::close(fp);

	// clients don't save follower info
	if ( multiplayer == CLIENT )
	{
		return 0;
	}

	if ( multiplayer == SINGLE )
	{
		strncpy(savefile, setSaveGameFileName(true, SaveFileType::FOLLOWERS, saveIndex).c_str(), PATH_MAX - 1);
	}
	else
	{
		strncpy(savefile, setSaveGameFileName(false, SaveFileType::FOLLOWERS, saveIndex).c_str(), PATH_MAX - 1);
	}
	completePath(path, savefile, outputdir);

	// now we save the follower information
	if ( (fp = FileIO::open(path, "wb")) == NULL )
	{
		printlog("warning: failed to save '%s'!\n", path);
		return 1;
	}
	fp->printf("BARONYSAVEGAMEFOLLOWERS");
	fp->printf(VERSION);

	// write follower information
	for ( int c = 0; c < MAXPLAYERS; c++ )
	{
		// record number of followers for this player
		Uint32 size = list_Size(&stats[c]->FOLLOWERS);
		fp->write(&size, sizeof(Uint32), 1);

		// get followerStats
		for ( int i = 0; i < size; i++ )
		{
			node_t* node = list_Node(&stats[c]->FOLLOWERS, i);
			if ( node )
			{
				Entity* follower = uidToEntity(*((Uint32*)node->element));
				Stat* followerStats = (follower) ? follower->getStats() : NULL;
				if ( followerStats )
				{
					// record follower stats
					fp->write(&followerStats->type, sizeof(Monster), 1);
					fp->write(&followerStats->sex, sizeof(sex_t), 1);
					fp->write(&followerStats->stat_appearance, sizeof(Uint32), 1);
					fp->write(followerStats->name, sizeof(char), 32);
					fp->write(&followerStats->HP, sizeof(Sint32), 1);
					fp->write(&followerStats->MAXHP, sizeof(Sint32), 1);
					fp->write(&followerStats->MP, sizeof(Sint32), 1);
					fp->write(&followerStats->MAXMP, sizeof(Sint32), 1);
					fp->write(&followerStats->STR, sizeof(Sint32), 1);
					fp->write(&followerStats->DEX, sizeof(Sint32), 1);
					fp->write(&followerStats->CON, sizeof(Sint32), 1);
					fp->write(&followerStats->INT, sizeof(Sint32), 1);
					fp->write(&followerStats->PER, sizeof(Sint32), 1);
					fp->write(&followerStats->CHR, sizeof(Sint32), 1);
					fp->write(&followerStats->EXP, sizeof(Sint32), 1);
					fp->write(&followerStats->LVL, sizeof(Sint32), 1);
					fp->write(&followerStats->GOLD, sizeof(Sint32), 1);
					fp->write(&followerStats->HUNGER, sizeof(Sint32), 1);

					for ( int j = 0; j < NUMPROFICIENCIES; j++ )
					{
						Sint32 val = followerStats->getProficiency(j);
						fp->write(&val, sizeof(Sint32), 1);
					}
					for ( int j = 0; j < NUMEFFECTS; j++ )
					{
						fp->write(&followerStats->EFFECTS[j], sizeof(bool), 1);
						fp->write(&followerStats->EFFECTS_TIMERS[j], sizeof(Sint32), 1);
					}
					for ( int j = 0; j < 32; ++j )
					{
						fp->write(&followerStats->MISC_FLAGS[j], sizeof(Sint32), 1);
					}

					Uint32 numAttributes = followerStats->attributes.size();
					fp->write(&numAttributes, sizeof(Uint32), 1);
					for ( auto& attribute : followerStats->attributes )
					{
						fp->write(attribute.first.c_str(), sizeof(char), 32);
						fp->write(attribute.second.c_str(), sizeof(char), 32);
					}

					// record follower inventory
					Uint32 invSize = list_Size(&followerStats->inventory);
					fp->write(&invSize, sizeof(Uint32), 1);
					for ( node_t* node = followerStats->inventory.first; node != NULL; node = node->next )
					{
						Item* item = (Item*)node->element;
						fp->write(&item->type, sizeof(ItemType), 1);
						fp->write(&item->status, sizeof(Status), 1);
						fp->write(&item->beatitude, sizeof(Sint16), 1);
						fp->write(&item->count, sizeof(Sint16), 1);
						fp->write(&item->appearance, sizeof(Uint32), 1);
						fp->write(&item->identified, sizeof(bool), 1);
						fp->write(&item->x, sizeof(Sint32), 1);
						fp->write(&item->y, sizeof(Sint32), 1);
					}

					// record follower equipment (since NPCs never store equipment as inventory)
					if ( followerStats->helmet )
					{
						Item* item = followerStats->helmet;
						fp->write(&item->type, sizeof(ItemType), 1);
						fp->write(&item->status, sizeof(Status), 1);
						fp->write(&item->beatitude, sizeof(Sint16), 1);
						fp->write(&item->count, sizeof(Sint16), 1);
						fp->write(&item->appearance, sizeof(Uint32), 1);
						fp->write(&item->identified, sizeof(bool), 1);
					}
					else
					{
						ItemType tempItem = static_cast<ItemType>(NUMITEMS);
						fp->write(&tempItem, sizeof(ItemType), 1);
					}
					if ( followerStats->breastplate )
					{
						Item* item = followerStats->breastplate;
						fp->write(&item->type, sizeof(ItemType), 1);
						fp->write(&item->status, sizeof(Status), 1);
						fp->write(&item->beatitude, sizeof(Sint16), 1);
						fp->write(&item->count, sizeof(Sint16), 1);
						fp->write(&item->appearance, sizeof(Uint32), 1);
						fp->write(&item->identified, sizeof(bool), 1);
					}
					else
					{
						ItemType tempItem = static_cast<ItemType>(NUMITEMS);
						fp->write(&tempItem, sizeof(ItemType), 1);
					}
					if ( followerStats->gloves )
					{
						Item* item = followerStats->gloves;
						fp->write(&item->type, sizeof(ItemType), 1);
						fp->write(&item->status, sizeof(Status), 1);
						fp->write(&item->beatitude, sizeof(Sint16), 1);
						fp->write(&item->count, sizeof(Sint16), 1);
						fp->write(&item->appearance, sizeof(Uint32), 1);
						fp->write(&item->identified, sizeof(bool), 1);
					}
					else
					{
						ItemType tempItem = static_cast<ItemType>(NUMITEMS);
						fp->write(&tempItem, sizeof(ItemType), 1);
					}
					if ( followerStats->shoes )
					{
						Item* item = followerStats->shoes;
						fp->write(&item->type, sizeof(ItemType), 1);
						fp->write(&item->status, sizeof(Status), 1);
						fp->write(&item->beatitude, sizeof(Sint16), 1);
						fp->write(&item->count, sizeof(Sint16), 1);
						fp->write(&item->appearance, sizeof(Uint32), 1);
						fp->write(&item->identified, sizeof(bool), 1);
					}
					else
					{
						ItemType tempItem = static_cast<ItemType>(NUMITEMS);
						fp->write(&tempItem, sizeof(ItemType), 1);
					}
					if ( followerStats->shield )
					{
						Item* item = followerStats->shield;
						fp->write(&item->type, sizeof(ItemType), 1);
						fp->write(&item->status, sizeof(Status), 1);
						fp->write(&item->beatitude, sizeof(Sint16), 1);
						fp->write(&item->count, sizeof(Sint16), 1);
						fp->write(&item->appearance, sizeof(Uint32), 1);
						fp->write(&item->identified, sizeof(bool), 1);
					}
					else
					{
						ItemType tempItem = static_cast<ItemType>(NUMITEMS);
						fp->write(&tempItem, sizeof(ItemType), 1);
					}
					if ( followerStats->weapon )
					{
						Item* item = followerStats->weapon;
						fp->write(&item->type, sizeof(ItemType), 1);
						fp->write(&item->status, sizeof(Status), 1);
						fp->write(&item->beatitude, sizeof(Sint16), 1);
						fp->write(&item->count, sizeof(Sint16), 1);
						fp->write(&item->appearance, sizeof(Uint32), 1);
						fp->write(&item->identified, sizeof(bool), 1);
					}
					else
					{
						ItemType tempItem = static_cast<ItemType>(NUMITEMS);
						fp->write(&tempItem, sizeof(ItemType), 1);
					}
					if ( followerStats->cloak )
					{
						Item* item = followerStats->cloak;
						fp->write(&item->type, sizeof(ItemType), 1);
						fp->write(&item->status, sizeof(Status), 1);
						fp->write(&item->beatitude, sizeof(Sint16), 1);
						fp->write(&item->count, sizeof(Sint16), 1);
						fp->write(&item->appearance, sizeof(Uint32), 1);
						fp->write(&item->identified, sizeof(bool), 1);
					}
					else
					{
						ItemType tempItem = static_cast<ItemType>(NUMITEMS);
						fp->write(&tempItem, sizeof(ItemType), 1);
					}
					if ( followerStats->amulet )
					{
						Item* item = followerStats->amulet;
						fp->write(&item->type, sizeof(ItemType), 1);
						fp->write(&item->status, sizeof(Status), 1);
						fp->write(&item->beatitude, sizeof(Sint16), 1);
						fp->write(&item->count, sizeof(Sint16), 1);
						fp->write(&item->appearance, sizeof(Uint32), 1);
						fp->write(&item->identified, sizeof(bool), 1);
					}
					else
					{
						ItemType tempItem = static_cast<ItemType>(NUMITEMS);
						fp->write(&tempItem, sizeof(ItemType), 1);
					}
					if ( followerStats->ring )
					{
						Item* item = followerStats->ring;
						fp->write(&item->type, sizeof(ItemType), 1);
						fp->write(&item->status, sizeof(Status), 1);
						fp->write(&item->beatitude, sizeof(Sint16), 1);
						fp->write(&item->count, sizeof(Sint16), 1);
						fp->write(&item->appearance, sizeof(Uint32), 1);
						fp->write(&item->identified, sizeof(bool), 1);
					}
					else
					{
						ItemType tempItem = static_cast<ItemType>(NUMITEMS);
						fp->write(&tempItem, sizeof(ItemType), 1);
					}
					if ( followerStats->mask )
					{
						Item* item = followerStats->mask;
						fp->write(&item->type, sizeof(ItemType), 1);
						fp->write(&item->status, sizeof(Status), 1);
						fp->write(&item->beatitude, sizeof(Sint16), 1);
						fp->write(&item->count, sizeof(Sint16), 1);
						fp->write(&item->appearance, sizeof(Uint32), 1);
						fp->write(&item->identified, sizeof(bool), 1);
					}
					else
					{
						ItemType tempItem = static_cast<ItemType>(NUMITEMS);
						fp->write(&tempItem, sizeof(ItemType), 1);
					}
				}
			}
		}
	}


	FileIO::close(fp);
	return 0;
}

/*-------------------------------------------------------------------------------

	loadGameOld

	Loads a character savegame stored in SAVEGAMEFILE

-------------------------------------------------------------------------------*/

int loadGameOld(int player, int saveIndex)
{
	File* fp;

	char savefile[PATH_MAX] = "";
	char path[PATH_MAX] = "";
	if ( multiplayer == SINGLE )
	{
		strncpy(savefile, setSaveGameFileName(true, SaveFileType::MAIN, saveIndex).c_str(), PATH_MAX - 1);
	}
	else
	{
		strncpy(savefile, setSaveGameFileName(false, SaveFileType::MAIN, saveIndex).c_str(), PATH_MAX - 1);
	}
	completePath(path, savefile, outputdir);

	// open file
	if ( (fp = FileIO::open(path, "rb")) == NULL )
	{
		printlog("error: failed to load '%s'!\n", path);
		return 1;
	}

	// read from file
	char checkstr[64];
	fp->read(checkstr, sizeof(char), strlen("BARONYSAVEGAME"));
	if ( strncmp(checkstr, "BARONYSAVEGAME", strlen("BARONYSAVEGAME")) )
	{
		printlog("error: '%s' is corrupt!\n", path);
		FileIO::close(fp);
		return 1;
	}
	fp->read(checkstr, sizeof(char), strlen(VERSION));
	int versionNumber = getSavegameVersion(checkstr);
	printlog("loadGameOld: '%s' version number %d", savefile, versionNumber);
	if ( versionNumber == -1 )
	{
		// if getSavegameVersion returned -1, abort.
		printlog("error: '%s' is corrupt!\n", path);
		FileIO::close(fp);
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
	fp->read(&uniqueGameKey, sizeof(Uint32), 1);
	local_rng.seedBytes(&uniqueGameKey, sizeof(uniqueGameKey));
	net_rng.seedBytes(&uniqueGameKey, sizeof(uniqueGameKey));

	Sint16 players_connected;
	fp->read(&players_connected, sizeof(Sint16), 1);
	Sint16 mul;
	fp->read(&mul, sizeof(Sint16), 1);

	if (players_connected == 0) {
	    if (mul == SINGLE) {
	        players_connected = 1;
	    } else {
	        players_connected =
	            (1 << 0) |
	            (1 << 1) |
	            (1 << 2) |
	            (1 << 3);
	    }
	}
	switch (mul) {
	default:
	case SINGLE: multiplayer = SINGLE; splitscreen = false; directConnect = false; break;
	case SERVER: multiplayer = SERVER; splitscreen = false; directConnect = false; break;
	case CLIENT: multiplayer = CLIENT; splitscreen = false; directConnect = false; break;
	case DIRECTSERVER: multiplayer = SERVER; splitscreen = false; directConnect = true; break;
	case DIRECTCLIENT: multiplayer = CLIENT; splitscreen = false; directConnect = true; break;
	case SERVERCROSSPLAY: multiplayer = SERVER; splitscreen = false; directConnect = false; break; // TODO!
	case SPLITSCREEN: multiplayer = SINGLE; splitscreen = true; directConnect = false; break;
	}

	if ( multiplayer == SINGLE )
	{
		for (int c = 0; c < MAXPLAYERS; ++c) {
			client_disconnected[c] = !(players_connected & (1<<c));
		}
	}

	fp->read(&clientnum, sizeof(Uint32), 1);
	fp->read(&mapseed, sizeof(Uint32), 1);
	fp->read(&currentlevel, sizeof(Uint32), 1);
	if ( versionNumber >= 323 )
	{
		loadedHash = (currentlevel & 0xFFFFFF00) >> 8;
		currentlevel = currentlevel & 0xFF;
	}
	fp->read(&secretlevel, sizeof(bool), 1);
	fp->read(&completionTime, sizeof(Uint32), 1);
	fp->read(&conductPenniless, sizeof(bool), 1);
	fp->read(&conductFoodless, sizeof(bool), 1);
	fp->read(&conductVegetarian, sizeof(bool), 1);
	fp->read(&conductIlliterate, sizeof(bool), 1);
	if ( versionNumber >= 310 )
	{
		for ( int c = 0; c < NUM_CONDUCT_CHALLENGES; ++c )
		{
			fp->read(&conductGameChallenges[c], sizeof(Sint32), 1);
		}
		for ( int c = 0; c < NUM_GAMEPLAY_STATISTICS; ++c )
		{
			fp->read(&gameStatistics[c], sizeof(Sint32), 1);
		}
	}
	if ( versionNumber >= 335 )
	{
		gameModeManager.currentSession.saveServerFlags();
		if ( multiplayer == CLIENT )
		{
			fp->read(&lobbyWindowSvFlags, sizeof(Uint32), 1);
		}
		else
		{
			fp->read(&svFlags, sizeof(Uint32), 1);
		}
		printlog("[SESSION]: Using savegame server flags");
	}

    // load hotbar and spells list
    Uint32 temp_hotbar[NUM_HOTBAR_SLOTS];
    if (splitscreen)
    {
        for (int c = 0; c < MAXPLAYERS; ++c)
        {
            if (c == player)
            {
	            // read hotbar item offsets
	            for ( int i = 0; i < NUM_HOTBAR_SLOTS; i++ )
	            {
		            fp->read(&temp_hotbar[i], sizeof(Uint32), 1);
	            }

	            // read spells
	            list_FreeAll(&players[c]->magic.spellList);
	            Uint32 numspells = 0;
	            fp->read(&numspells, sizeof(Uint32), 1);
	            for ( int s = 0; s < numspells; ++s )
	            {
		            int spellnum = 0;
		            fp->read(&spellnum, sizeof(Uint32), 1);
		            spell_t* spell = copySpell(getSpellFromID(spellnum));

		            node_t* node = list_AddNodeLast(&players[c]->magic.spellList);
		            node->element = spell;
		            node->deconstructor = &spellDeconstructor;
		            node->size = sizeof(spell);
	            }

				clientLearnedAlchemyRecipes[c].clear();
				if ( versionNumber >= 381 )
				{
					// read alchemy recipes
					Uint32 numrecipes = 0;
					fp->read(&numrecipes, sizeof(Uint32), 1);
					for ( int r = 0; r < numrecipes; ++r )
					{
						std::pair<int, std::pair<int, int>> recipeEntry;
						fp->read(&recipeEntry.first, sizeof(Sint32), 1);
						fp->read(&recipeEntry.second.first, sizeof(Sint32), 1);
						fp->read(&recipeEntry.second.second, sizeof(Sint32), 1);
						clientLearnedAlchemyRecipes[c].push_back(recipeEntry);
					}
				}

				clientLearnedScrollLabels[c].clear();
				if ( versionNumber >= 382 )
				{
					// read scroll labels
					Uint32 numscrolls = 0;
					fp->read(&numscrolls, sizeof(Uint32), 1);
					for ( int s = 0; s < numscrolls; ++s )
					{
						int scroll = 0;
						fp->read(&scroll, sizeof(Sint32), 1);
						clientLearnedScrollLabels[c].insert(scroll);
					}
				}
            }
            else
            {
	            fp->seek(sizeof(Uint32) * NUM_HOTBAR_SLOTS, File::SeekMode::ADD);

                int numspells = 0;
                fp->read(&numspells, sizeof(Uint32), 1);
                for ( int i = 0; i < numspells; i++ )
                {
	                fp->seek(sizeof(Uint32), File::SeekMode::ADD);
                }

				if ( versionNumber >= 381 )
				{
					// read alchemy recipes
					Uint32 numrecipes = 0;
					fp->read(&numrecipes, sizeof(Uint32), 1);
					for ( int r = 0; r < numrecipes; ++r )
					{
						fp->seek(sizeof(Sint32), File::SeekMode::ADD);
						fp->seek(sizeof(Sint32), File::SeekMode::ADD);
						fp->seek(sizeof(Sint32), File::SeekMode::ADD);
					}
				}

				if ( versionNumber >= 382 )
				{
					// read scroll labels
					Uint32 numscrolls = 0;
					fp->read(&numscrolls, sizeof(Uint32), 1);
					for ( int s = 0; s < numscrolls; ++s )
					{
						fp->seek(sizeof(Sint32), File::SeekMode::ADD);
					}
				}
            }
        }
    }
    else
    {
	    // read hotbar item offsets
	    for ( int c = 0; c < NUM_HOTBAR_SLOTS; c++ )
	    {
		    fp->read(&temp_hotbar[c], sizeof(Uint32), 1);
	    }

	    // read spells
	    list_FreeAll(&players[player]->magic.spellList);
	    Uint32 numspells = 0;
	    fp->read(&numspells, sizeof(Uint32), 1);
	    for ( int c = 0; c < numspells; c++ )
	    {
		    int spellnum = 0;
		    fp->read(&spellnum, sizeof(Uint32), 1);
		    spell_t* spell = copySpell(getSpellFromID(spellnum));

		    node_t* node = list_AddNodeLast(&players[player]->magic.spellList);
		    node->element = spell;
		    node->deconstructor = &spellDeconstructor;
		    node->size = sizeof(spell);
	    }

		clientLearnedAlchemyRecipes[player].clear();
		if ( versionNumber >= 381 )
		{
			// read alchemy recipes
			Uint32 numrecipes = 0;
			fp->read(&numrecipes, sizeof(Uint32), 1);
			for ( int r = 0; r < numrecipes; ++r )
			{
				std::pair<int, std::pair<int, int>> recipeEntry;
				fp->read(&recipeEntry.first, sizeof(Sint32), 1);
				fp->read(&recipeEntry.second.first, sizeof(Sint32), 1);
				fp->read(&recipeEntry.second.second, sizeof(Sint32), 1);
				clientLearnedAlchemyRecipes[player].push_back(recipeEntry);
			}
		}

		clientLearnedScrollLabels[player].clear();
		if ( versionNumber >= 382 )
		{
			// read scroll labels
			Uint32 numscrolls = 0;
			fp->read(&numscrolls, sizeof(Uint32), 1);
			for ( int s = 0; s < numscrolls; ++s )
			{
				int scroll = 0;
				fp->read(&scroll, sizeof(Sint32), 1);
				clientLearnedScrollLabels[player].insert(scroll);
			}
		}
    }

	int monsters = NUMMONSTERS;
	if ( versionNumber < 325 )
	{
		monsters = 33;
	}

	// skip through other player data until you get to the correct player
	for ( int c = 0; c < player; c++ )
	{
		fp->seek(sizeof(Uint32), File::SeekMode::ADD);
		fp->seek(monsters * sizeof(Sint32), File::SeekMode::ADD);
		fp->seek(sizeof(Monster), File::SeekMode::ADD);
		fp->seek(sizeof(sex_t), File::SeekMode::ADD);
		fp->seek(sizeof(Uint32), File::SeekMode::ADD);
		fp->seek(sizeof(char) * 32, File::SeekMode::ADD);
		fp->seek(sizeof(Sint32), File::SeekMode::ADD);
		fp->seek(sizeof(Sint32), File::SeekMode::ADD);
		fp->seek(sizeof(Sint32), File::SeekMode::ADD);
		fp->seek(sizeof(Sint32), File::SeekMode::ADD);
		fp->seek(sizeof(Sint32), File::SeekMode::ADD);
		fp->seek(sizeof(Sint32), File::SeekMode::ADD);
		fp->seek(sizeof(Sint32), File::SeekMode::ADD);
		fp->seek(sizeof(Sint32), File::SeekMode::ADD);
		fp->seek(sizeof(Sint32), File::SeekMode::ADD);
		fp->seek(sizeof(Sint32), File::SeekMode::ADD);
		fp->seek(sizeof(Sint32), File::SeekMode::ADD);
		fp->seek(sizeof(Sint32), File::SeekMode::ADD);
		fp->seek(sizeof(Sint32), File::SeekMode::ADD);
		fp->seek(sizeof(Sint32), File::SeekMode::ADD);

		if ( versionNumber >= 323 )
		{
			fp->seek( sizeof(Sint32)*NUMPROFICIENCIES, File::SeekMode::ADD);
		}
		else
		{
			fp->seek(sizeof(Sint32)*14, File::SeekMode::ADD);
		}

		if ( versionNumber <= 323 ) // legacy
		{
			fp->seek(sizeof(bool)*32, File::SeekMode::ADD);
			fp->seek(sizeof(Sint32)*32, File::SeekMode::ADD);
		}
		else
		{
			fp->seek(sizeof(bool)*NUMEFFECTS, File::SeekMode::ADD);
			fp->seek(sizeof(Sint32)*NUMEFFECTS, File::SeekMode::ADD);
		}

		if ( versionNumber >= 323 )
		{
			fp->seek(sizeof(Sint32)*32, File::SeekMode::ADD); // stat flags
		}

        if ( multiplayer == SINGLE )
        {
			int numitems = 0;
			fp->read(&numitems, sizeof(Uint32), 1);
			for ( int i = 0; i < numitems; i++ )
			{
				fp->seek(sizeof(ItemType), File::SeekMode::ADD);
				fp->seek(sizeof(Status), File::SeekMode::ADD);
				fp->seek(sizeof(Sint16), File::SeekMode::ADD);
				fp->seek(sizeof(Sint16), File::SeekMode::ADD);
				fp->seek(sizeof(Uint32), File::SeekMode::ADD);
				fp->seek(sizeof(bool), File::SeekMode::ADD);
				fp->seek(sizeof(Sint32), File::SeekMode::ADD);
				fp->seek(sizeof(Sint32), File::SeekMode::ADD);
			}
			fp->seek(sizeof(Uint32) * 10, File::SeekMode::ADD); // equipment slots
        }
		else if ( multiplayer == SERVER )
		{
		    if ( c == 0 ) {
				// server needs to skip past its own inventory
				int numitems = 0;
				fp->read(&numitems, sizeof(Uint32), 1);

				for ( int i = 0; i < numitems; i++ )
				{
					fp->seek(sizeof(ItemType), File::SeekMode::ADD);
					fp->seek(sizeof(Status), File::SeekMode::ADD);
					fp->seek(sizeof(Sint16), File::SeekMode::ADD);
					fp->seek(sizeof(Sint16), File::SeekMode::ADD);
					fp->seek(sizeof(Uint32), File::SeekMode::ADD);
					fp->seek(sizeof(bool), File::SeekMode::ADD);
					fp->seek(sizeof(Sint32), File::SeekMode::ADD);
					fp->seek(sizeof(Sint32), File::SeekMode::ADD);
				}
				fp->seek(sizeof(Uint32) * 10, File::SeekMode::ADD); // equipment slots
		    } else {
			    // server needs to skip past other players' equipment
			    // (this is stored differently)
			    for ( int i = 0; i < 10; i++ )
			    {
				    int itemtype = NUMITEMS;
				    fp->read(&itemtype, sizeof(ItemType), 1);
				    if ( itemtype < NUMITEMS )
				    {
					    fp->seek(sizeof(Status), File::SeekMode::ADD);
					    fp->seek(sizeof(Sint16), File::SeekMode::ADD);
					    fp->seek(sizeof(Sint16), File::SeekMode::ADD);
					    fp->seek(sizeof(Uint32), File::SeekMode::ADD);
					    fp->seek(sizeof(bool), File::SeekMode::ADD);
				    }
			    }
			}
		}
		else if ( multiplayer == CLIENT )
		{
			// client needs only to skip the dummy byte
			fp->seek(sizeof(ItemType), File::SeekMode::ADD);
		}
	}

	// read in player data
	stats[player]->clearStats();
	fp->read(&client_classes[player], sizeof(Uint32), 1);
	for ( int c = 0; c < monsters; c++ )
	{
		fp->read(&kills[c], sizeof(Sint32), 1);
	}
	fp->read(&stats[player]->type, sizeof(Monster), 1);
	fp->read(&stats[player]->sex, sizeof(sex_t), 1);
	fp->read(&stats[player]->stat_appearance, sizeof(Uint32), 1);
	if ( versionNumber >= 323 )
	{
		stats[player]->playerRace = ((stats[player]->stat_appearance & 0xFF00) >> 8);
		stats[player]->stat_appearance = (stats[player]->stat_appearance & 0xFF);
	}
	fp->read(&stats[player]->name, sizeof(char), 32);
	fp->read(&stats[player]->HP, sizeof(Sint32), 1);
	fp->read(&stats[player]->MAXHP, sizeof(Sint32), 1);
	fp->read(&stats[player]->MP, sizeof(Sint32), 1);
	fp->read(&stats[player]->MAXMP, sizeof(Sint32), 1);
	fp->read(&stats[player]->STR, sizeof(Sint32), 1);
	fp->read(&stats[player]->DEX, sizeof(Sint32), 1);
	fp->read(&stats[player]->CON, sizeof(Sint32), 1);
	fp->read(&stats[player]->INT, sizeof(Sint32), 1);
	fp->read(&stats[player]->PER, sizeof(Sint32), 1);
	fp->read(&stats[player]->CHR, sizeof(Sint32), 1);
	fp->read(&stats[player]->EXP, sizeof(Sint32), 1);
	fp->read(&stats[player]->LVL, sizeof(Sint32), 1);
	fp->read(&stats[player]->GOLD, sizeof(Sint32), 1);
	fp->read(&stats[player]->HUNGER, sizeof(Sint32), 1);
	for ( int c = 0; c < NUMPROFICIENCIES; c++ )
	{
		if ( versionNumber < 323 && c >= PRO_UNARMED )
		{
			stats[player]->setProficiency(c, 0);
		}
		else
		{
			Sint32 val = 0;
			fp->read(&val, sizeof(Sint32), 1);
			stats[player]->setProficiency(c, val);
		}
	}
	for ( int c = 0; c < NUMEFFECTS; c++ )
	{
		if ( versionNumber <= 323 ) // legacy
		{
			if ( c < 32 )
			{
				fp->read(&stats[player]->EFFECTS[c], sizeof(bool), 1);
				fp->read(&stats[player]->EFFECTS_TIMERS[c], sizeof(Sint32), 1);
			}
			else
			{
				stats[player]->EFFECTS[c] = false;
				stats[player]->EFFECTS_TIMERS[c] = 0;
			}
		}
		else
		{
			fp->read(&stats[player]->EFFECTS[c], sizeof(bool), 1);
			fp->read(&stats[player]->EFFECTS_TIMERS[c], sizeof(Sint32), 1);
		}
	}
	if ( versionNumber >= 323 )
	{
		for ( int c = 0; c < 32; c++ )
		{
			fp->read(&stats[player]->MISC_FLAGS[c], sizeof(Sint32), 1);
			if ( c < STAT_FLAG_PLAYER_RACE )
			{
				stats[player]->MISC_FLAGS[c] = 0; // we don't really need these on load.
			}
		}
	}

	if ( players[player]->isLocalPlayer() )
	{
		// inventory
		int numitems = 0;
		fp->read(&numitems, sizeof(Uint32), 1);
		stats[player]->inventory.first = NULL;
		stats[player]->inventory.last = NULL;
		for ( int c = 0; c < numitems; c++ )
		{
			ItemType type;
			Status status;
			Sint16 beatitude;
			Sint16 count;
			Uint32 appearance;
			bool identified;
			fp->read(&type, sizeof(ItemType), 1);
			fp->read(&status, sizeof(Status), 1);
			fp->read(&beatitude, sizeof(Sint16), 1);
			fp->read(&count, sizeof(Sint16), 1);
			fp->read(&appearance, sizeof(Uint32), 1);
			fp->read(&identified, sizeof(bool), 1);
			Item* item = newItem(type, status, beatitude, count, appearance, identified, &stats[player]->inventory);
			fp->read(&item->x, sizeof(Sint32), 1);
			fp->read(&item->y, sizeof(Sint32), 1);
		}

		int c;
		node_t* node;

		// equipment
		fp->read(&c, sizeof(Uint32), 1);
		node = list_Node(&stats[player]->inventory, c);
		if ( node )
		{
			stats[player]->helmet = (Item*)node->element;
		}
		else
		{
			stats[player]->helmet = NULL;
		}
		fp->read(&c, sizeof(Uint32), 1);
		node = list_Node(&stats[player]->inventory, c);
		if ( node )
		{
			stats[player]->breastplate = (Item*)node->element;
		}
		else
		{
			stats[player]->breastplate = NULL;
		}
		fp->read(&c, sizeof(Uint32), 1);
		node = list_Node(&stats[player]->inventory, c);
		if ( node )
		{
			stats[player]->gloves = (Item*)node->element;
		}
		else
		{
			stats[player]->gloves = NULL;
		}
		fp->read(&c, sizeof(Uint32), 1);
		node = list_Node(&stats[player]->inventory, c);
		if ( node )
		{
			stats[player]->shoes = (Item*)node->element;
		}
		else
		{
			stats[player]->shoes = NULL;
		}
		fp->read(&c, sizeof(Uint32), 1);
		node = list_Node(&stats[player]->inventory, c);
		if ( node )
		{
			stats[player]->shield = (Item*)node->element;
		}
		else
		{
			stats[player]->shield = NULL;
		}
		fp->read(&c, sizeof(Uint32), 1);
		node = list_Node(&stats[player]->inventory, c);
		if ( node )
		{
			stats[player]->weapon = (Item*)node->element;
		}
		else
		{
			stats[player]->weapon = NULL;
		}
		fp->read(&c, sizeof(Uint32), 1);
		node = list_Node(&stats[player]->inventory, c);
		if ( node )
		{
			stats[player]->cloak = (Item*)node->element;
		}
		else
		{
			stats[player]->cloak = NULL;
		}
		fp->read(&c, sizeof(Uint32), 1);
		node = list_Node(&stats[player]->inventory, c);
		if ( node )
		{
			stats[player]->amulet = (Item*)node->element;
		}
		else
		{
			stats[player]->amulet = NULL;
		}
		fp->read(&c, sizeof(Uint32), 1);
		node = list_Node(&stats[player]->inventory, c);
		if ( node )
		{
			stats[player]->ring = (Item*)node->element;
		}
		else
		{
			stats[player]->ring = NULL;
		}
		fp->read(&c, sizeof(Uint32), 1);
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
			for ( int c = 0; c < 10; c++ )
			{
				ItemType type;
				Status status;
				Sint16 beatitude;
				Sint16 count;
				Uint32 appearance;
				bool identified;

				fp->read(&type, sizeof(ItemType), 1);
				if ( (int)type < NUMITEMS )
				{
					fp->read(&status, sizeof(Status), 1);
					fp->read(&beatitude, sizeof(Sint16), 1);
					fp->read(&count, sizeof(Sint16), 1);
					fp->read(&appearance, sizeof(Uint32), 1);
					fp->read(&identified, sizeof(bool), 1);

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
	auto& hotbar = players[player]->hotbar.slots();
	auto& hotbar_alternate = players[player]->hotbar.slotsAlternate();
	for ( int c = 0; c < NUM_HOTBAR_SLOTS; c++ )
	{
		node_t* node = list_Node(&stats[player]->inventory, temp_hotbar[c]);
		if ( node )
		{
			Item* item = (Item*)node->element;
			hotbar[c].item = item->uid;
		}
		else
		{
			hotbar[c].item = 0;
			hotbar[c].resetLastItem();
			for ( int d = 0; d < NUM_HOTBAR_ALTERNATES; ++d )
			{
				hotbar_alternate[d][c].item = 0;
				hotbar_alternate[d][c].resetLastItem();
			}
		}
	}

	// reset some unused variables
	stats[player]->monster_sound = NULL;
	stats[player]->monster_idlevar = 0;
	stats[player]->leader_uid = 0;
	stats[player]->FOLLOWERS.first = NULL;
	stats[player]->FOLLOWERS.last = NULL;


	hash += (stats[clientnum]->STR + stats[clientnum]->LVL + stats[clientnum]->DEX * stats[clientnum]->INT);
	hash += (stats[clientnum]->CON * stats[clientnum]->PER + std::min(stats[clientnum]->GOLD, 5000) - stats[clientnum]->CON);
	hash += (stats[clientnum]->HP - stats[clientnum]->MP);
	hash += (currentlevel);

	if ( hash != loadedHash )
	{
		gameStatistics[STATISTICS_DISABLE_UPLOAD] = 1;
	}
	//printlog("%d, %d", hash, loadedHash);

    {
	    enchantedFeatherScrollsShuffled.clear();
	    enchantedFeatherScrollsShuffled.reserve(enchantedFeatherScrollsFixedList.size());
	    auto shuffle = enchantedFeatherScrollsFixedList;
		BaronyRNG feather_rng;
		feather_rng.seedBytes(&uniqueGameKey, sizeof(uniqueGameKey));
	    while (!shuffle.empty()) {
	        int index = feather_rng.getU8() % shuffle.size();
	        enchantedFeatherScrollsShuffled.push_back(shuffle[index]);
	        shuffle.erase(shuffle.begin() + index);
	    }
	}

	FileIO::close(fp);
	return 0;
}

/*-------------------------------------------------------------------------------

	loadGameFollowersOld

	Loads follower data from a save game file

-------------------------------------------------------------------------------*/

list_t* loadGameFollowersOld(int saveIndex)
{
	File* fp;
	int c;

	char savefile[PATH_MAX] = "";
	char path[PATH_MAX] = "";
	if ( multiplayer == SINGLE )
	{
		strncpy(savefile, setSaveGameFileName(true, SaveFileType::FOLLOWERS, saveIndex).c_str(), PATH_MAX - 1);
	}
	else
	{
		strncpy(savefile, setSaveGameFileName(false, SaveFileType::FOLLOWERS, saveIndex).c_str(), PATH_MAX - 1);
	}
	completePath(path, savefile, outputdir);

	// open file
	if ( (fp = FileIO::open(path, "rb")) == NULL )
	{
		printlog("error: failed to load '%s'!\n", path);
		return NULL;
	}

	// read from file
	char checkstr[64];
	fp->read(checkstr, sizeof(char), strlen("BARONYSAVEGAMEFOLLOWERS"));
	if ( strncmp(checkstr, "BARONYSAVEGAMEFOLLOWERS", strlen("BARONYSAVEGAMEFOLLOWERS")) )
	{
		printlog("error: '%s' is corrupt!\n", path);
		FileIO::close(fp);
		return NULL;
	}
	fp->read(checkstr, sizeof(char), strlen(VERSION));
	int versionNumber = getSavegameVersion(checkstr);
	printlog("loadGameFollowersOld: '%s' version number %d", savefile, versionNumber);
	if ( versionNumber == -1 )
	{
		// if version number returned is invalid, abort
		printlog("error: '%s' is corrupt!\n", path);
		FileIO::close(fp);
		return nullptr;
	}

	// create followers list
	list_t* followers = (list_t*) malloc(sizeof(list_t));
	followers->first = NULL;
	followers->last = NULL;

	// read the follower data
	for ( int c = 0; c < MAXPLAYERS; c++ )
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
		fp->read(&numFollowers, sizeof(Uint32), 1);

		for ( int i = 0; i < numFollowers; i++ )
		{
			// Stat set to 0 as monster type not needed, values will be overwritten by the saved follower data
			Stat* followerStats = new Stat(0);

			node_t* node = list_AddNodeLast(followerList);
			node->element = followerStats;
			node->deconstructor = &statDeconstructor;
			node->size = sizeof(followerStats);

			// read follower attributes
			fp->read(&followerStats->type, sizeof(Monster), 1);
			fp->read(&followerStats->sex, sizeof(sex_t), 1);
			fp->read(&followerStats->stat_appearance, sizeof(Uint32), 1);
			fp->read(&followerStats->name, sizeof(char), 32);
			fp->read(&followerStats->HP, sizeof(Sint32), 1);
			fp->read(&followerStats->MAXHP, sizeof(Sint32), 1);
			fp->read(&followerStats->MP, sizeof(Sint32), 1);
			fp->read(&followerStats->MAXMP, sizeof(Sint32), 1);
			fp->read(&followerStats->STR, sizeof(Sint32), 1);
			fp->read(&followerStats->DEX, sizeof(Sint32), 1);
			fp->read(&followerStats->CON, sizeof(Sint32), 1);
			fp->read(&followerStats->INT, sizeof(Sint32), 1);
			fp->read(&followerStats->PER, sizeof(Sint32), 1);
			fp->read(&followerStats->CHR, sizeof(Sint32), 1);
			fp->read(&followerStats->EXP, sizeof(Sint32), 1);
			fp->read(&followerStats->LVL, sizeof(Sint32), 1);
			fp->read(&followerStats->GOLD, sizeof(Sint32), 1);
			fp->read(&followerStats->HUNGER, sizeof(Sint32), 1);

			for ( int j = 0; j < NUMPROFICIENCIES; j++ )
			{
				if ( versionNumber < 323 && j >= PRO_UNARMED )
				{
					followerStats->setProficiency(j, 0);
				}
				else
				{
					Sint32 val = 0;
					fp->read(&val, sizeof(Sint32), 1);
					followerStats->setProficiency(j, val);
				}
			}
			for ( int j = 0; j < NUMEFFECTS; j++ )
			{
				if ( versionNumber <= 323 ) // legacy
				{
					if ( c < 32 )
					{
						fp->read(&followerStats->EFFECTS[j], sizeof(bool), 1);
						fp->read(&followerStats->EFFECTS_TIMERS[j], sizeof(Sint32), 1);
					}
					else
					{
						followerStats->EFFECTS[j] = false;
						followerStats->EFFECTS_TIMERS[j] = 0;
					}
				}
				else
				{
					fp->read(&followerStats->EFFECTS[j], sizeof(bool), 1);
					fp->read(&followerStats->EFFECTS_TIMERS[j], sizeof(Sint32), 1);
				}
			}
			if ( versionNumber >= 323 )
			{
				for ( int j = 0; j < 32; ++j )
				{
					fp->read(&followerStats->MISC_FLAGS[j], sizeof(Sint32), 1);
				}
			}
			if ( versionNumber >= 384 )
			{
				Uint32 numAttributes = 0;
				fp->read(&numAttributes, sizeof(Uint32), 1);
				while ( numAttributes > 0 )
				{
					char key[32];
					memset(key, 0, sizeof(key));
					char value[32];
					memset(value, 0, sizeof(value));
					fp->read(&key, sizeof(char), 32);
					fp->read(&value, sizeof(char), 32);
					followerStats->attributes.emplace(std::make_pair(key, value));
					--numAttributes;
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
			fp->read(&invSize, sizeof(Uint32), 1);
			for ( int j = 0; j < invSize; j++ )
			{
				fp->read(&type, sizeof(ItemType), 1);
				fp->read(&status, sizeof(Status), 1);
				fp->read(&beatitude, sizeof(Sint16), 1);
				fp->read(&count, sizeof(Sint16), 1);
				fp->read(&appearance, sizeof(Uint32), 1);
				fp->read(&identified, sizeof(bool), 1);

				Item* item = newItem(type, status, beatitude, count, appearance, identified, &followerStats->inventory);
				fp->read(&item->x, sizeof(Sint32), 1);
				fp->read(&item->y, sizeof(Sint32), 1);
			}

			// read follower equipment
			for ( int b = 0; b < 10; b++ )
			{
				fp->read(&type, sizeof(ItemType), 1);
				if ( (int)type < NUMITEMS )
				{
					fp->read(&status, sizeof(Status), 1);
					fp->read(&beatitude, sizeof(Sint16), 1);
					fp->read(&count, sizeof(Sint16), 1);
					fp->read(&appearance, sizeof(Uint32), 1);
					fp->read(&identified, sizeof(bool), 1);

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

	FileIO::close(fp);
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
	int result = 0;

    for (int c = 0; c < static_cast<int>(SaveFileType::SIZE_OF_TYPE); ++c)
    {
	    if ( gametype == SINGLE )
	    {
		    strncpy(savefile, setSaveGameFileName(true, static_cast<SaveFileType>(c), saveIndex).c_str(), PATH_MAX - 1);
	    }
	    else
	    {
		    strncpy(savefile, setSaveGameFileName(false, static_cast<SaveFileType>(c), saveIndex).c_str(), PATH_MAX - 1);
	    }
	    completePath(path, savefile, outputdir);
	    if (access(path, F_OK) != -1)
	    {
		    printlog("deleting savegame in '%s'...\n", path);
		    int r = remove(path);
		    if (r)
		    {
		        result |= r;
			    printlog("warning: failed to delete savegame in '%s'!\n", path);
#ifdef _MSC_VER
			    printlog(strerror(errno));
#endif
		    }
	    }
	}

	return result;
}

/*-------------------------------------------------------------------------------

	saveGameExists

	checks to see if a valid save game exists.

-------------------------------------------------------------------------------*/

bool saveGameExists(bool singleplayer, int saveIndex)
{
	char path[PATH_MAX] = "";
	auto savefile = setSaveGameFileName(singleplayer, SaveFileType::JSON, saveIndex);
	completePath(path, savefile.c_str(), outputdir);

	if (access(path, F_OK ) == -1) {
		return false;
	} else {
		SaveGameInfo info;
		if (!FileHelper::readObject(path, info)) {
			return false;
		}

		if (info.magic_cookie != "BARONYJSONSAVE") {
			return false;
		}

		if (info.game_version == -1) {
			return false;
		}

		return true;
	}
}

/*-------------------------------------------------------------------------------

	getSaveGameInfo

	Fill a struct with info about a savegame slot

-------------------------------------------------------------------------------*/

SaveGameInfo getSaveGameInfo(bool singleplayer, int saveIndex)
{
	char path[PATH_MAX] = "";
	auto savefile = setSaveGameFileName(singleplayer, SaveFileType::JSON, saveIndex);
	completePath(path, savefile.c_str(), outputdir);

	// read info object, check file read succeeded
	SaveGameInfo info;
	bool result = FileHelper::readObject(path, info);
	if (!result) {
		info.game_version = -1;
	}
	
	// check hash
	Uint32 hash = 0;
	struct tm* tm = nullptr;
#ifdef WINDOWS
	struct _stat s;
	if (_stat(path, &s) == 0) {
		tm = localtime(&s.st_mtime);
	}
#else
	struct stat s;
	if (stat(path, &s) == 0) {
		tm = localtime(&s.st_mtime);
	}
#endif
	if (tm) {
		hash = tm->tm_hour + tm->tm_mday * tm->tm_year + tm->tm_wday + tm->tm_yday;
	}
	if (info.players.size() > info.player_num) {
		if ( info.game_version < 410 )
		{
			auto& stats = info.players[info.player_num].stats;
			hash += stats.STR + stats.LVL + stats.DEX * stats.INT;
			hash += stats.CON * stats.PER + std::min(stats.GOLD, 5000) - stats.CON;
			hash += stats.HP - stats.MP;
			hash += info.dungeon_lvl;
		}
		else
		{
			info.computeHash(info.player_num, hash);
		}
	}
	if (hash != info.hash) {
		info.hash = 0;
	}

	return info;
}

/*-------------------------------------------------------------------------------

	getSaveGameName

	Gets the name of the saved game

-------------------------------------------------------------------------------*/

const char* getSaveGameName(const SaveGameInfo& info)
{
    return info.gamename.c_str();
}

/*-------------------------------------------------------------------------------

getSaveGameVersionNum

Returns the savefile version

-------------------------------------------------------------------------------*/

int getSaveGameVersionNum(const SaveGameInfo& info)
{
	return info.game_version;
}

/*-------------------------------------------------------------------------------

	getSaveGameType

	Returns the multiplayer variable stored in the save game

-------------------------------------------------------------------------------*/

int getSaveGameType(const SaveGameInfo& info)
{
	return info.multiplayer_type;
}

/*-------------------------------------------------------------------------------

	getSaveGameClientnum

	Returns the clientnum variable stored in the save game

-------------------------------------------------------------------------------*/

int getSaveGameClientnum(const SaveGameInfo& info)
{
	return info.player_num;
}

/*-------------------------------------------------------------------------------

	getSaveGameMapSeed

	Returns the mapseed variable stored in the save game

-------------------------------------------------------------------------------*/

Uint32 getSaveGameMapSeed(const SaveGameInfo& info)
{
	return info.mapseed;
}

int getSavegameVersion(const char* checkstr)
{
	const int maxlen = (int)strlen(VERSION);
	int versionNumber = 300;
	char versionStr[4] = "000";
	int i = 0;
	for ( int j = 0; checkstr[j] != '\0' && j < maxlen; ++j )
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
		achievementRhythmOfTheKnight[c] = false;
		achievementStatusStrobe[c] = false;
		achievementStatusThankTheTank[c] = false;
		achievementRhythmOfTheKnightVec[c].clear();
		achievementThankTheTankPair[c].clear();
		achievementStrobeVec[c].clear();
		achievementStatusBaitAndSwitch[c] = false;
		achievementBaitAndSwitchTimer[c] = 0;
		playerFailedRangedOnlyConduct[c] = false;
		clientLearnedAlchemyIngredients[c].clear();
		clientLearnedAlchemyRecipes[c].clear();
		clientLearnedScrollLabels[c].clear();
	}
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
			if ( multiplayer != SINGLE || splitscreen )
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
	if ( ItemTooltips_t::itemsJsonHashRead != ItemTooltips_t::kItemsJsonHash )
	{
		conductGameChallenges[CONDUCT_MODDED] = 1;
		Mods::disableSteamAchievements = true;
	}
	if ( !conductGameChallenges[CONDUCT_MODDED_NO_ACHIEVEMENTS] )
	{
		if ( Mods::disableSteamAchievements
			|| (multiplayer == CLIENT && Mods::lobbyDisableSteamAchievements) )
		{
			conductGameChallenges[CONDUCT_MODDED_NO_ACHIEVEMENTS] = 1;
		}
	}
	if ( !conductGameChallenges[CONDUCT_MULTIPLAYER] )
	{
		if ( multiplayer != SINGLE || splitscreen )
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
		if ( Mods::numCurrentModsLoaded >= 0 )
		{
			conductGameChallenges[CONDUCT_MODDED] = 1;
			//Mods::disableSteamAchievements = true;
		}
	}
	if ( conductGameChallenges[CONDUCT_MODDED_NO_ACHIEVEMENTS]
		|| conductGameChallenges[CONDUCT_CHEATS_ENABLED]
		|| conductGameChallenges[CONDUCT_LIFESAVING] )
	{
		gameStatistics[STATISTICS_DISABLE_UPLOAD] = 1;
	}
	if ( !gameModeManager.allowsGlobalHiscores() )
	{
		gameStatistics[STATISTICS_DISABLE_UPLOAD] = 1;
	}

	achievementObserver.updateClientBounties(false);

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

	if ( gameStatistics[STATISTICS_ALCHEMY_RECIPES] != 0 && clientLearnedAlchemyIngredients[clientnum].empty() )
	{
		int numpotions = static_cast<int>(potionStandardAppearanceMap.size());
		for ( int i = 0; i < numpotions; ++i )
		{
			bool learned = gameStatistics[STATISTICS_ALCHEMY_RECIPES] & (1 << i);
			if ( learned )
			{
				auto typeAppearance = potionStandardAppearanceMap.at(i);
				int type = typeAppearance.first;
				clientLearnedAlchemyIngredients[clientnum].insert(type);
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
			if ( !learned && (GenericGUI[clientnum].isItemBaseIngredient(type) || GenericGUI[clientnum].isItemSecondaryIngredient(type)) )
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
		if ( stats[clientnum]->stat_appearance == 0 && (stats[clientnum]->type == INCUBUS || stats[clientnum]->type == SUCCUBUS) )
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
				if ( GenericGUI[clientnum].tinkeringGetCraftingCost(item, &dummy1, &dummy2) )
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

std::string setSaveGameFileName(bool singleplayer, SaveFileType type, int saveIndex)
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

	if ( type == SaveFileType::MAIN )
	{
		if ( singleplayer )
		{
			if ( Mods::numCurrentModsLoaded == -1 )
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
			if ( Mods::numCurrentModsLoaded == -1 )
			{
				filename.append("_mp.dat");
			}
			else
			{
				filename.append("_mp_modded.dat");
			}
		}
	}
	else if ( type == SaveFileType::FOLLOWERS )
	{
		if ( singleplayer )
		{
			if ( Mods::numCurrentModsLoaded == -1 )
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
			if ( Mods::numCurrentModsLoaded == -1 )
			{
				filename.append("_mp_npcs.dat");
			}
			else
			{
				filename.append("_mp_npcs_modded.dat");
			}
		}
	}
	else if ( type == SaveFileType::SCREENSHOT )
	{
		if ( singleplayer )
		{
			if ( Mods::numCurrentModsLoaded == -1 )
			{
				filename.append("_screenshot.png");
			}
			else
			{
				filename.append("_screenshot_modded.png");
			}
		}
		else
		{
			if ( Mods::numCurrentModsLoaded == -1 )
			{
				filename.append("_mp_screenshot.png");
			}
			else
			{
				filename.append("_mp_screenshot_modded.png");
			}
		}
		filename.insert(0, "/");
		filename.insert(0, outputdir);
	}
	else if ( type == SaveFileType::JSON )
	{
		if ( singleplayer )
		{
			if ( true/*Mods::numCurrentModsLoaded == -1*/ )
			{
				filename.append(".baronysave");
			}
			else
			{
				filename.append("_modded.baronysave");
			}
		}
		else
		{
			if ( true/*Mods::numCurrentModsLoaded == -1*/ )
			{
				filename.append("_mp.baronysave");
			}
			else
			{
				filename.append("_mp_modded.baronysave");
			}
		}
	}

	return filename;
}

bool anySaveFileExists(bool singleplayer)
{
	for ( int fileNumber = 0; fileNumber < SAVE_GAMES_MAX; ++fileNumber )
	{
		if ( saveGameExists(singleplayer, fileNumber) )
		{
			return true;
		}
	}
	return false;
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
	if ( multiplayer == CLIENT
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
							if ( !achievementRhythmOfTheKnight[player] )
							{
								steamAchievementClient(player, "BARONY_ACH_RHYTHM_OF_THE_KNIGHT");
								achievementRhythmOfTheKnight[player] = true;
							}
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

	if ( stats[player]->playerRace == RACE_SUCCUBUS && stats[player]->stat_appearance != 0 )
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
	if ( !target || target->behavior != &actMonster )
	{
		return;
	}
	if ( multiplayer == CLIENT )
	{
		return;
	}


	auto& entry = achievementThankTheTankPair[player][target->getUID()];
	if ( !targetKilled )
	{
		entry = ticks;
		//messagePlayer(0, "pair: %d, %d", achievementThankTheTankPair[player].first, achievementThankTheTankPair[player].second);
	}
	else if ( entry != 0 ) // check there is a ticks/UID entry.
	{
		if ( players[player] && players[player]->entity )
		{
			if ( players[player]->entity->checkEnemy(target) )
			{
				// check timestamp within 3 seconds.
				if ( (ticks - entry) / 50.f < 3.f )
				{
					achievementStatusThankTheTank[player] = true;
					achievementThankTheTankPair[player].erase(target->getUID());
					if ( players[player]->mechanics.allowedRaiseBlockingAgainstEntity(*target) )
					{
						int skillLVL = 3 * (stats[player]->getProficiency(PRO_SHIELD) / 20);
						if ( local_rng.rand() % (5 + skillLVL) == 0 )
						{
							bool increase = true;
							if ( stats[player]->shield && itemCategory(stats[player]->shield) != ARMOR )
							{
								if ( stats[player]->getProficiency(PRO_SHIELD) >= SKILL_LEVEL_SKILLED )
								{
									increase = false;
								}
							}
							if ( increase )
							{
								players[player]->entity->increaseSkill(PRO_SHIELD);
								players[player]->mechanics.enemyRaisedBlockingAgainst[target->getUID()]++;
							}
						}
					}
				}
			}
		}
	}
}

#ifdef STEAMWORKS

bool steamLeaderboardSetScore(score_t* score)
{
	return false;
//	if ( !g_SteamLeaderboards )
//	{
//		return false;
//	}
//
//	if ( !score )
//	{
//		return false;
//	}
//
//	if ( score->victory == 0 )
//	{
//		return false;
//	}
//
//	if ( score->conductGameChallenges[CONDUCT_CHEATS_ENABLED] 
//		|| Mods::disableSteamAchievements
//		|| score->conductGameChallenges[CONDUCT_LIFESAVING] )
//	{
//		return false;
//	}
//	if ( score->gameStatistics[STATISTICS_DISABLE_UPLOAD] == 1 )
//	{
//		return false;
//	}
//	
//	bool monster = false;
//	if ( score->stats && score->stats->playerRace > 0 && score->stats->appearance == 0 )
//	{
//		monster = true;
//	}
//
//	if ( !score->conductGameChallenges[CONDUCT_MULTIPLAYER] )
//	{
//		// single player
//		if ( !score->conductGameChallenges[CONDUCT_HARDCORE] )
//		{
//			if ( score->victory == 2 )
//			{
//				if ( monster )
//				{
//					g_SteamLeaderboards->LeaderboardUpload.boardIndex = LEADERBOARD_DLC_HELL_TIME;
//				}
//				else
//				{
//					g_SteamLeaderboards->LeaderboardUpload.boardIndex = LEADERBOARD_HELL_TIME;
//				}
//			}
//			else if ( score->victory == 3 || score->victory == 4 || score->victory == 5 )
//			{
//				if ( monster )
//				{
//					g_SteamLeaderboards->LeaderboardUpload.boardIndex = LEADERBOARD_DLC_NORMAL_TIME;
//				}
//				else
//				{
//					g_SteamLeaderboards->LeaderboardUpload.boardIndex = LEADERBOARD_NORMAL_TIME;
//				}
//			}
//			else if ( score->victory == 1 )
//			{
//				if ( monster )
//				{
//					g_SteamLeaderboards->LeaderboardUpload.boardIndex = LEADERBOARD_DLC_CLASSIC_TIME;
//				}
//				else
//				{
//					g_SteamLeaderboards->LeaderboardUpload.boardIndex = LEADERBOARD_CLASSIC_TIME;
//				}
//			}
//		}
//		else if ( score->conductGameChallenges[CONDUCT_HARDCORE] )
//		{
//			if ( score->victory == 3 || score->victory == 4 || score->victory == 5 )
//			{
//				if ( monster )
//				{
//					g_SteamLeaderboards->LeaderboardUpload.boardIndex = LEADERBOARD_DLC_HARDCORE_TIME;
//				}
//				else
//				{
//					g_SteamLeaderboards->LeaderboardUpload.boardIndex = LEADERBOARD_HARDCORE_TIME;
//				}
//			}
//			else
//			{
//				if ( monster )
//				{
//					g_SteamLeaderboards->LeaderboardUpload.boardIndex = LEADERBOARD_DLC_CLASSIC_HARDCORE_TIME;
//				}
//				else
//				{
//					g_SteamLeaderboards->LeaderboardUpload.boardIndex = LEADERBOARD_CLASSIC_HARDCORE_TIME;
//				}
//			}
//		}
//	}
//	else if ( score->conductGameChallenges[CONDUCT_MULTIPLAYER] )
//	{
//		// multiplayer
//		if ( score->victory == 2 )
//		{
//			if ( monster )
//			{
//				g_SteamLeaderboards->LeaderboardUpload.boardIndex = LEADERBOARD_DLC_MULTIPLAYER_HELL_TIME;
//			}
//			else
//			{
//				g_SteamLeaderboards->LeaderboardUpload.boardIndex = LEADERBOARD_MULTIPLAYER_HELL_TIME;
//			}
//		}
//		else if ( score->victory == 3 || score->victory == 4 || score->victory == 5 )
//		{
//			if ( monster )
//			{
//				g_SteamLeaderboards->LeaderboardUpload.boardIndex = LEADERBOARD_DLC_MULTIPLAYER_TIME;
//			}
//			else
//			{
//				g_SteamLeaderboards->LeaderboardUpload.boardIndex = LEADERBOARD_MULTIPLAYER_TIME;
//			}
//		}
//		else if ( score->victory == 1 )
//		{
//			if ( monster )
//			{
//				g_SteamLeaderboards->LeaderboardUpload.boardIndex = LEADERBOARD_DLC_MULTIPLAYER_CLASSIC_TIME;
//			}
//			else
//			{
//				g_SteamLeaderboards->LeaderboardUpload.boardIndex = LEADERBOARD_MULTIPLAYER_CLASSIC_TIME;
//			}
//		}
//	}
//	else
//	{
//		g_SteamLeaderboards->LeaderboardUpload.boardIndex = LEADERBOARD_NONE;
//	}
//
//	if ( g_SteamLeaderboards->LeaderboardUpload.boardIndex == LEADERBOARD_NONE )
//	{
//		return false;
//	}
//
//	// assemble the score tags.
//	//int completionTime = score->completionTime;
//	int c = 0;
//	int tag = TAG_MONSTER_KILLS_1;
//	int i = 0;
//	int tagWidth = 8;
//	for ( int c = 0; c < NUMMONSTERS; ++c )
//	{
//		g_SteamLeaderboards->LeaderboardUpload.tags[tag] |= (static_cast<Uint8>(score->kills[c]) << (i * tagWidth));
//		++i;
//		if ( i >((32 / tagWidth) - 1) )
//		{
//			i = 0;
//			++tag;
//		}
//	}
//
//	i = 0;
//	tagWidth = 8;
//	tag = TAG_NAME1;
//	for ( int c = 0; c < std::min(32, (int)(strlen(score->stats->name))); ++c )
//	{
//		g_SteamLeaderboards->LeaderboardUpload.tags[tag] |= (Uint8)(score->stats->name[c]) << (i * tagWidth);
//		++i;
//		if ( i > ((32 / tagWidth) - 1) )
//		{
//			i = 0;
//			++tag;
//		}
//	}
//
//	tagWidth = 8;
//	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_RACESEXAPPEARANCECLASS] |= score->stats->type;
//	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_RACESEXAPPEARANCECLASS] |= (score->stats->sex) << (tagWidth * 1);
//	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_RACESEXAPPEARANCECLASS] |= (score->stats->appearance) << (tagWidth * 2);
//	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_RACESEXAPPEARANCECLASS] |= (score->classnum) << (tagWidth * 3);
//
//	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_VICTORYDUNGEONLEVELCONDUCTORIGINAL] |= (score->victory);
//	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_VICTORYDUNGEONLEVELCONDUCTORIGINAL] |= (score->dungeonlevel) << (tagWidth);
//	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_VICTORYDUNGEONLEVELCONDUCTORIGINAL] |= (score->conductPenniless) << (tagWidth * 2);
//	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_VICTORYDUNGEONLEVELCONDUCTORIGINAL] |= (score->conductFoodless) << (tagWidth * 2 + 1);
//	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_VICTORYDUNGEONLEVELCONDUCTORIGINAL] |= (score->conductVegetarian) << (tagWidth * 2 + 2);
//	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_VICTORYDUNGEONLEVELCONDUCTORIGINAL] |= (score->conductIlliterate) << (tagWidth * 2 + 3);
//
//	tag = TAG_CONDUCT_2W_1;
//	tagWidth = 16;
//	i = 0;
//	for ( int c = 0; c < 32; ++c )
//	{
//		if ( c < 16 )
//		{
//			g_SteamLeaderboards->LeaderboardUpload.tags[tag] |= score->conductGameChallenges[c] << (c * 2);
//		}
//		else
//		{
//			g_SteamLeaderboards->LeaderboardUpload.tags[tag] |= score->conductGameChallenges[c] << ((16 - c) * 2);
//		}
//		if ( c == 15 )
//		{
//			++tag;
//		}
//	}
//
//	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_CONDUCT_4W_1] |= (Uint8)(score->stats->playerRace); // store in right-most 8 bits.
//	// conducts TAG_CONDUCT_4W_2 to TAG_CONDUCT_4W_4 unused.
//
//	// store new gameplay stats as required. not many to start with.
//	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_GAMEPLAY_STATS_2W_1] |= std::min(3, score->gameStatistics[STATISTICS_FIRE_MAYBE_DIFFERENT]);
//	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_GAMEPLAY_STATS_2W_1] |= std::min(3, score->gameStatistics[STATISTICS_SITTING_DUCK]) << 2;
//	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_GAMEPLAY_STATS_2W_1] |= std::min(3, score->gameStatistics[STATISTICS_TEMPT_FATE]) << 4;
//
//	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_GAMEPLAY_STATS_8W_1] |= (Uint8)score->gameStatistics[STATISTICS_BOMB_SQUAD];
//	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_GAMEPLAY_STATS_8W_1] |= (Uint8)score->gameStatistics[STATISTICS_HOT_TUB_TIME_MACHINE] << 8;
//	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_GAMEPLAY_STATS_8W_1] |= (Uint8)score->gameStatistics[STATISTICS_YES_WE_CAN] << 16;
//
//	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_GAMEPLAY_STATS_16W_1] |= (Uint16)score->gameStatistics[STATISTICS_HEAL_BOT];
//
//	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_HEALTH] |= (Uint16)score->stats->MAXHP;
//	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_HEALTH] |= (Uint16)score->stats->HP << 16;
//
//	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_MANA] |= (Uint16)score->stats->MAXMP;
//	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_MANA] |= (Uint16)score->stats->MP << 16;
//
//	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_STRDEXCONINT] |= (Uint8)score->stats->STR;
//	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_STRDEXCONINT] |= (Uint8)score->stats->DEX << 8;
//	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_STRDEXCONINT] |= (Uint8)score->stats->CON << 16;
//	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_STRDEXCONINT] |= (Uint8)score->stats->INT << 24;
//
//	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_PERCHREXPLVL] |= (Uint8)score->stats->PER;
//	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_PERCHREXPLVL] |= (Uint8)score->stats->CHR << 8;
//	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_PERCHREXPLVL] |= (Uint8)score->stats->EXP << 16;
//	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_PERCHREXPLVL] |= (Uint8)score->stats->LVL << 24;
//
//	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_GOLD] |= score->stats->GOLD;
//
//	tagWidth = 8;
//	tag = TAG_PROFICIENCY1;
//	i = 0;
//	for ( int c = 0; c < NUMPROFICIENCIES; ++c )
//	{
//		g_SteamLeaderboards->LeaderboardUpload.tags[tag] |= score->stats->PROFICIENCIES[c] << (i * tagWidth);
//		++i;
//		if ( i > ((32 / tagWidth) - 1) )
//		{
//			i = 0;
//			++tag;
//		}
//	}
//
//	if ( score->stats->helmet )
//	{
//		g_SteamLeaderboards->LeaderboardUpload.tags[TAG_EQUIPMENT1] |= (Uint8)(score->stats->helmet->type);
//		g_SteamLeaderboards->LeaderboardUpload.tags[TAG_EQUIPMENT_BEATITUDE1] |= (Sint16)(score->stats->helmet->beatitude);
//		g_SteamLeaderboards->LeaderboardUpload.tags[TAG_EQUIPMENT_APPEARANCE] |=
//			(score->stats->helmet->appearance % items[score->stats->helmet->type].variations);
//	}
//	if ( score->stats->breastplate )
//	{
//		g_SteamLeaderboards->LeaderboardUpload.tags[TAG_EQUIPMENT1] |= (Uint8)(score->stats->breastplate->type) << 8;
//		g_SteamLeaderboards->LeaderboardUpload.tags[TAG_EQUIPMENT_BEATITUDE1] |= (Sint16)(score->stats->breastplate->beatitude) << 8;
//		g_SteamLeaderboards->LeaderboardUpload.tags[TAG_EQUIPMENT_APPEARANCE] |=
//			(score->stats->breastplate->appearance % items[score->stats->breastplate->type].variations) << 8;
//	}
//	if ( score->stats->gloves )
//	{
//		g_SteamLeaderboards->LeaderboardUpload.tags[TAG_EQUIPMENT1] |= (Uint8)(score->stats->gloves->type) << 16;
//		g_SteamLeaderboards->LeaderboardUpload.tags[TAG_EQUIPMENT_BEATITUDE1] |= (Sint16)(score->stats->gloves->beatitude) << 16;
//	}
//	if ( score->stats->shoes )
//	{
//		g_SteamLeaderboards->LeaderboardUpload.tags[TAG_EQUIPMENT1] |= (Uint8)(score->stats->shoes->type) << 24;
//		g_SteamLeaderboards->LeaderboardUpload.tags[TAG_EQUIPMENT_BEATITUDE1] |= (Sint16)(score->stats->shoes->beatitude) << 24;
//	}
//
//	if ( score->stats->shield )
//	{
//		g_SteamLeaderboards->LeaderboardUpload.tags[TAG_EQUIPMENT2] |= (Uint8)(score->stats->shield->type);
//		g_SteamLeaderboards->LeaderboardUpload.tags[TAG_EQUIPMENT_BEATITUDE2] |= (Sint16)(score->stats->shield->beatitude);
//		g_SteamLeaderboards->LeaderboardUpload.tags[TAG_EQUIPMENT_APPEARANCE] |=
//			(score->stats->shield->appearance % items[score->stats->shield->type].variations) << 12;
//	}
//	if ( score->stats->weapon )
//	{
//		g_SteamLeaderboards->LeaderboardUpload.tags[TAG_EQUIPMENT2] |= (Uint8)(score->stats->weapon->type) << 8;
//		g_SteamLeaderboards->LeaderboardUpload.tags[TAG_EQUIPMENT_BEATITUDE2] |= (Sint16)(score->stats->weapon->beatitude) << 8;
//	}
//	if ( score->stats->cloak )
//	{
//		g_SteamLeaderboards->LeaderboardUpload.tags[TAG_EQUIPMENT2] |= (Uint8)(score->stats->cloak->type) << 16;
//		g_SteamLeaderboards->LeaderboardUpload.tags[TAG_EQUIPMENT_BEATITUDE2] |= (Sint16)(score->stats->cloak->beatitude) << 16;
//		g_SteamLeaderboards->LeaderboardUpload.tags[TAG_EQUIPMENT_APPEARANCE] |= 
//			(score->stats->cloak->appearance % items[score->stats->cloak->type].variations) << 4;
//	}
//	if ( score->stats->amulet )
//	{
//		g_SteamLeaderboards->LeaderboardUpload.tags[TAG_EQUIPMENT2] |= (Uint8)(score->stats->amulet->type) << 24;
//		g_SteamLeaderboards->LeaderboardUpload.tags[TAG_EQUIPMENT_BEATITUDE2] |= (Sint16)(score->stats->amulet->beatitude) << 16;
//	}
//
//	if ( score->stats->ring )
//	{
//		g_SteamLeaderboards->LeaderboardUpload.tags[TAG_EQUIPMENT3] |= (Uint8)(score->stats->ring->type);
//		g_SteamLeaderboards->LeaderboardUpload.tags[TAG_EQUIPMENT_BEATITUDE3] |= (Sint16)(score->stats->ring->beatitude);
//	}
//	if ( score->stats->mask )
//	{
//		g_SteamLeaderboards->LeaderboardUpload.tags[TAG_EQUIPMENT3] |= (Uint8)(score->stats->mask->type) << 8;
//		g_SteamLeaderboards->LeaderboardUpload.tags[TAG_EQUIPMENT_BEATITUDE3] |= (Sint16)(score->stats->mask->beatitude);
//	}
//
//	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_TOTAL_SCORE] = totalScore(score);
//	g_SteamLeaderboards->LeaderboardUpload.tags[TAG_COMPLETION_TIME] = score->completionTime / TICKS_PER_SECOND;
//	return true;
}

bool steamLeaderboardReadScore(int tags[CSteamLeaderboards::k_numLeaderboardTags])
{
	return false;
	//stats[0]->clearStats();

	//int c = 0;
	//int tag = TAG_MONSTER_KILLS_1;
	//int tagWidth = 8;
	//int i = 0;
	//for ( int c = 0; c < NUMMONSTERS; c++ )
	//{
	//	kills[c] = ((tags[tag]) >> (i * tagWidth)) & 0xFF;
	//	++i;
	//	if ( i > ((32 / tagWidth) - 1) )
	//	{
	//		i = 0;
	//		++tag;
	//	}
	//}

	//i = 0;
	//tagWidth = 8;
	//tag = TAG_NAME1;
	//char name[33] = "";
	//for ( int c = 0; c < 32; ++c )
	//{
	//	name[c] = ((tags[tag]) >> (i * tagWidth)) & 0xFF;
	//	if ( name[c] == 0 )
	//	{
	//		break;
	//	}
	//	++i;
	//	if ( i > ((32 / tagWidth) - 1) )
	//	{
	//		i = 0;
	//		++tag;
	//	}
	//}
	//name[c] = '\0';
	//strcpy(stats[0]->name, name);


	//tagWidth = 8;
	//stats[0]->type = (Monster)(tags[TAG_RACESEXAPPEARANCECLASS] & 0xFF);
	//stats[0]->sex = (sex_t)((tags[TAG_RACESEXAPPEARANCECLASS] >> tagWidth) & 0xFF);
	//stats[0]->appearance = (tags[TAG_RACESEXAPPEARANCECLASS] >> tagWidth * 2) & 0xFF;
	//client_classes[0] = (tags[TAG_RACESEXAPPEARANCECLASS] >> tagWidth * 3) & 0xFF;

	//victory = (tags[TAG_VICTORYDUNGEONLEVELCONDUCTORIGINAL] >> tagWidth * 0) & 0xFF;
	//currentlevel = (tags[TAG_VICTORYDUNGEONLEVELCONDUCTORIGINAL] >> tagWidth * 1) & 0xFF;
	//conductPenniless = (tags[TAG_VICTORYDUNGEONLEVELCONDUCTORIGINAL] >> tagWidth * 2) & 1;
	//conductFoodless = (tags[TAG_VICTORYDUNGEONLEVELCONDUCTORIGINAL] >> (tagWidth * 2 + 1)) & 1;
	//conductVegetarian = (tags[TAG_VICTORYDUNGEONLEVELCONDUCTORIGINAL] >> (tagWidth * 2 + 2)) & 1;
	//conductIlliterate = (tags[TAG_VICTORYDUNGEONLEVELCONDUCTORIGINAL] >> (tagWidth * 2 + 3)) & 1;

	//tag = TAG_CONDUCT_2W_1;
	//tagWidth = 2;
	//i = 0;
	//for ( int c = 0; c < 32; ++c )
	//{
	//	if ( c < 16 )
	//	{
	//		conductGameChallenges[c] = (tags[tag] >> c * tagWidth) & 0b11;
	//	}
	//	else
	//	{
	//		conductGameChallenges[c] = (tags[tag] >> (16 - c) * tagWidth) & 0b11;
	//	}
	//	if ( c == 15 )
	//	{
	//		++tag;
	//	}
	//}

	//stats[0]->playerRace = (tags[TAG_CONDUCT_4W_1] & 0xFF);

	//// conducts TAG_CONDUCT_4W_2 to TAG_CONDUCT_4W_4 unused.

	//// store new gameplay stats as required. not many to start with.
	//gameStatistics[STATISTICS_FIRE_MAYBE_DIFFERENT] = tags[TAG_GAMEPLAY_STATS_2W_1] & 0b11;
	//gameStatistics[STATISTICS_SITTING_DUCK] = (tags[TAG_GAMEPLAY_STATS_2W_1] >> 2) & 0b11;
	//gameStatistics[STATISTICS_TEMPT_FATE] = (tags[TAG_GAMEPLAY_STATS_2W_1] >> 4) & 0b11;

	//gameStatistics[STATISTICS_BOMB_SQUAD] = tags[TAG_GAMEPLAY_STATS_8W_1] & 0xFF;
	//gameStatistics[STATISTICS_HOT_TUB_TIME_MACHINE] = (tags[TAG_GAMEPLAY_STATS_8W_1] >> 8) & 0xFF;
	//gameStatistics[STATISTICS_YES_WE_CAN] = (tags[TAG_GAMEPLAY_STATS_8W_1] >> 16) & 0xFF;

	//gameStatistics[STATISTICS_HEAL_BOT] = tags[TAG_GAMEPLAY_STATS_16W_1] & 0xFFFF;

	//stats[0]->MAXHP = tags[TAG_HEALTH] & 0xFFFF;
	//stats[0]->HP = (tags[TAG_HEALTH] >> 16) & 0xFFFF;
	//stats[0]->MAXMP = tags[TAG_MANA] & 0xFFFF;
	//stats[0]->MP = (tags[TAG_MANA] >> 16) & 0xFFFF;
	//stats[0]->STR = (tags[TAG_STRDEXCONINT] >> 0) & 0xFF;
	//if ( stats[0]->STR > 240 )
	//{
	//	stats[0]->STR = (Sint8)stats[0]->STR;
	//}
	//stats[0]->DEX = (tags[TAG_STRDEXCONINT] >> 8) & 0xFF;
	//if ( stats[0]->DEX > 240 )
	//{
	//	stats[0]->DEX = (Sint8)stats[0]->DEX;
	//}
	//stats[0]->CON = (tags[TAG_STRDEXCONINT] >> 16) & 0xFF;
	//if ( stats[0]->CON > 240 )
	//{
	//	stats[0]->CON = (Sint8)stats[0]->CON;
	//}
	//stats[0]->INT = (tags[TAG_STRDEXCONINT] >> 24) & 0xFF;
	//if ( stats[0]->INT > 240 )
	//{
	//	stats[0]->INT = (Sint8)stats[0]->INT;
	//}
	//stats[0]->PER = (tags[TAG_PERCHREXPLVL] >> 0) & 0xFF;
	//if ( stats[0]->PER > 240 )
	//{
	//	stats[0]->PER = (Sint8)stats[0]->PER;
	//}
	//stats[0]->CHR = (tags[TAG_PERCHREXPLVL] >> 8) & 0xFF;
	//if ( stats[0]->CHR > 240 )
	//{
	//	stats[0]->CHR = (Sint8)stats[0]->CHR;
	//}
	//stats[0]->EXP = (tags[TAG_PERCHREXPLVL] >> 16) & 0xFF;
	//stats[0]->LVL = (tags[TAG_PERCHREXPLVL] >> 24) & 0xFF;
	//stats[0]->GOLD = tags[TAG_GOLD];

	//tagWidth = 8;
	//tag = TAG_PROFICIENCY1;
	//i = 0;
	//for ( int c = 0; c < NUMPROFICIENCIES; ++c )
	//{
	//	stats[0]->PROFICIENCIES[c] = (tags[tag] >> (i * tagWidth)) & 0xFF;
	//	++i;
	//	if ( i > ((32 / tagWidth) - 1) )
	//	{
	//		i = 0;
	//		++tag;
	//	}
	//}

	//list_FreeAll(&stats[0]->inventory);
	//if ( ((tags[TAG_EQUIPMENT1] >> 0) & 0xFF) > 0 )
	//{
	//	stats[0]->helmet = newItem(ItemType((tags[TAG_EQUIPMENT1] >> 0) & 0xFF), EXCELLENT, 
	//		Sint16((tags[TAG_EQUIPMENT_BEATITUDE1] >> 0) & 0xFF), 1, tags[TAG_EQUIPMENT_APPEARANCE] & 0xF, true, &stats[0]->inventory);
	//}
	//if ( ((tags[TAG_EQUIPMENT1] >> 8) & 0xFF) > 0 )
	//{
	//	stats[0]->breastplate = newItem(ItemType((tags[TAG_EQUIPMENT1] >> 8) & 0xFF), EXCELLENT,
	//		Sint16((tags[TAG_EQUIPMENT_BEATITUDE1] >> 8) & 0xFF), 1, (tags[TAG_EQUIPMENT_APPEARANCE] >> 8) & 0xF, true, &stats[0]->inventory);
	//}
	//if ( ((tags[TAG_EQUIPMENT1] >> 16) & 0xFF) > 0 )
	//{
	//	stats[0]->gloves = newItem(ItemType((tags[TAG_EQUIPMENT1] >> 16) & 0xFF), EXCELLENT, Sint16((tags[TAG_EQUIPMENT_BEATITUDE1] >> 16) & 0xFF), 1, local_rng.rand(), true, &stats[0]->inventory);
	//}
	//if ( ((tags[TAG_EQUIPMENT1] >> 24) & 0xFF) > 0 )
	//{
	//	stats[0]->shoes = newItem(ItemType((tags[TAG_EQUIPMENT1] >> 24) & 0xFF), EXCELLENT, Sint16((tags[TAG_EQUIPMENT_BEATITUDE1] >> 24) & 0xFF), 1, local_rng.rand(), true, &stats[0]->inventory);
	//}

	//if ( ((tags[TAG_EQUIPMENT2] >> 0) & 0xFF) > 0 )
	//{
	//	stats[0]->shield = newItem(ItemType((tags[TAG_EQUIPMENT2] >> 0) & 0xFF), EXCELLENT, 
	//		Sint16((tags[TAG_EQUIPMENT_BEATITUDE2] >> 0) & 0xFF), 1, (tags[TAG_EQUIPMENT_APPEARANCE] >> 12) & 0xF, true, &stats[0]->inventory);
	//}
	//if ( ((tags[TAG_EQUIPMENT2] >> 8) & 0xFF) > 0 )
	//{
	//	stats[0]->weapon = newItem(ItemType((tags[TAG_EQUIPMENT2] >> 8) & 0xFF), EXCELLENT, Sint16((tags[TAG_EQUIPMENT_BEATITUDE2] >> 8) & 0xFF), 1, local_rng.rand(), true, &stats[0]->inventory);
	//}
	//if ( ((tags[TAG_EQUIPMENT2] >> 16) & 0xFF) > 0 )
	//{
	//	stats[0]->cloak = newItem(ItemType((tags[TAG_EQUIPMENT2] >> 16) & 0xFF), EXCELLENT, 
	//		Sint16((tags[TAG_EQUIPMENT_BEATITUDE2] >> 16) & 0xFF), 1, (tags[TAG_EQUIPMENT_APPEARANCE] >> 4) & 0xF, true, &stats[0]->inventory);
	//}
	//if ( ((tags[TAG_EQUIPMENT2] >> 24) & 0xFF) > 0 )
	//{
	//	stats[0]->amulet = newItem(ItemType((tags[TAG_EQUIPMENT2] >> 24) & 0xFF), EXCELLENT, Sint16((tags[TAG_EQUIPMENT_BEATITUDE2] >> 24) & 0xFF), 1, local_rng.rand(), true, &stats[0]->inventory);
	//}

	//if ( ((tags[TAG_EQUIPMENT3] >> 16) & 0xFF) > 0 )
	//{
	//	stats[0]->ring = newItem(ItemType((tags[TAG_EQUIPMENT3] >> 16) & 0xFF), EXCELLENT, Sint16((tags[TAG_EQUIPMENT_BEATITUDE3] >> 0) & 0xFF), 1, local_rng.rand(), true, &stats[0]->inventory);
	//}
	//if ( ((tags[TAG_EQUIPMENT3] >> 24) & 0xFF) > 0 )
	//{
	//	stats[0]->mask = newItem(ItemType((tags[TAG_EQUIPMENT3] >> 24) & 0xFF), EXCELLENT, Sint16((tags[TAG_EQUIPMENT_BEATITUDE3] >> 8) & 0xFF), 1, local_rng.rand(), true, &stats[0]->inventory);
	//}

	//completionTime = tags[TAG_COMPLETION_TIME] * TICKS_PER_SECOND;
	//g_SteamLeaderboards->LeaderboardUpload.tags[TAG_TOTAL_SCORE] = totalScore(score);
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
			playerAchievements[i].phantomMaskFirstStrikes.clear();
			playerAchievements[i].bountyTargets.clear();
			playerAchievements[i].updatedBountyTargets = false;
			playerAchievements[i].wearingBountyHat = false;
			playerAchievements[i].totalKillsTickUpdate = false;
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

void AchievementObserver::updateClientBounties(bool firstSend)
{
	bool bountyLost = false;
	if ( multiplayer != CLIENT )
	{
		// check if any bounties had become followers, then remove them
		for ( int c = 0; c < MAXPLAYERS; ++c )
		{
			for ( node_t* node = stats[c]->FOLLOWERS.first; node != nullptr; node = node->next )
			{
				if ( (Uint32*)node->element )
				{
					Uint32 uid = *((Uint32*)node->element);
					for ( int d = 0; d < MAXPLAYERS; ++d )
					{
						if ( playerAchievements[d].bountyTargets.find(uid)
							!= playerAchievements[d].bountyTargets.end() )
						{
							bountyLost = true;
							playerAchievements[d].bountyTargets.erase(uid);
							
							if ( stats[d] && stats[d]->helmet && stats[d]->helmet->type == HAT_BOUNTYHUNTER )
							{
								// failed to finish your bounty
								messagePlayerColor(d, MESSAGE_COMBAT | MESSAGE_HINT, makeColorRGB(255, 0, 0), Language::get(6102));
							}
						}
					}
				}
			}
		}
	}

	if ( multiplayer == SERVER )
	{
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			if ( client_disconnected[i] ) { continue; }

			bool hat = false;
			if ( stats[i] && stats[i]->helmet && stats[i]->helmet->type == HAT_BOUNTYHUNTER )
			{
				hat = true;
			}

			if ( hat != playerAchievements[i].wearingBountyHat )
			{
				playerAchievements[i].wearingBountyHat = hat;

				for ( int c = 1; c < MAXPLAYERS; ++c )
				{
					strcpy((char*)net_packet->data, "BNTH");
					net_packet->data[4] = i;
					net_packet->data[5] = playerAchievements[i].wearingBountyHat ? 1 : 0;
					net_packet->address.host = net_clients[c - 1].host;
					net_packet->address.port = net_clients[c - 1].port;
					net_packet->len = 6;
					sendPacketSafe(net_sock, -1, net_packet, c - 1);
				}
			}

			if ( i == 0 )
			{
				continue;
			}

			if ( !firstSend )
			{
				bool update = false;
				if ( !playerAchievements[i].updatedBountyTargets )
				{
					if ( hat )
					{
						update = true;
						playerAchievements[i].updatedBountyTargets = true;
					}
				}
				else if ( bountyLost )
				{
					update = true;
				}
				else if ( ticks % (13 * TICKS_PER_SECOND) == 0 )
				{
					update = true;
				}

				if ( !update )
				{
					continue;
				}
			}

			for ( int c = 0; c < MAXPLAYERS; ++c )
			{
				auto& bounties = playerAchievements[c].bountyTargets;
				strcpy((char*)net_packet->data, "BNTY");
				net_packet->data[4] = c;
				net_packet->data[5] = (Uint8)bounties.size();
				int index = 6;
				for ( auto uid : bounties )
				{
					SDLNet_Write32(uid, &net_packet->data[index]);
					index += 4;
				}
				net_packet->address.host = net_clients[i - 1].host;
				net_packet->address.port = net_clients[i - 1].port;
				net_packet->len = 6 + (bounties.size() * 4);
				sendPacketSafe(net_sock, -1, net_packet, i - 1);
			}
		}
	}
}

void AchievementObserver::updateData()
{
	PlayerAchievements::allPlayersDeadEvent = false;
	getCurrentPlayerUids();
	if ( updateOnLevelChange() )
	{
		entityAchievementsToProcess.clear();
#ifdef DEBUG_ACHIEVEMENTS
		messagePlayer(0, MESSAGE_DEBUG, "[DEBUG]: Achievement data reset for floor.");
#endif
	}

	std::vector<Entity*> monstersGeneratedOnLevel;
	for ( node_t* node = map.creatures->first; node; node = node->next )
	{
		Entity* mapCreature = (Entity*)node->element;
		if ( mapCreature && mapCreature->behavior == &actMonster )
		{
			if ( auto stats = mapCreature->getStats() )
			{
				if ( stats->type == SPELLBOT
					|| stats->type == SENTRYBOT
					|| stats->type == DUMMYBOT
					|| stats->type == GYROBOT )
				{
					continue;
				}
			}
			monstersGeneratedOnLevel.push_back(mapCreature);
		}
	}

	BaronyRNG bountySeed;
	bountySeed.seedBytes(&mapseed, sizeof(mapseed));
	if ( multiplayer != CLIENT )
	{
		std::vector<Entity*> bountyTargets;
		for ( auto monster : monstersGeneratedOnLevel )
		{
			bountyTargets.push_back(monster);
		}
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			achievementObserver.playerAchievements[i].bountyTargets.clear();
			if ( client_disconnected[i] )
			{
				continue;
			}
			if ( bountyTargets.size() > 0 )
			{
				size_t index = bountySeed.rand() % bountyTargets.size();
				achievementObserver.playerAchievements[i].bountyTargets.insert(bountyTargets[index]->getUID());
				bountyTargets.erase(bountyTargets.begin() + index);

				if ( stats[i]->helmet && stats[i]->helmet->type == HAT_BOUNTYHUNTER )
				{
					if ( stats[i]->helmet->beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[i]) )
					{
						if ( abs(stats[i]->helmet->beatitude) >= 2 )
						{
							if ( bountyTargets.size() > 0 )
							{
								// additional bounty
								index = bountySeed.rand() % bountyTargets.size();
								achievementObserver.playerAchievements[i].bountyTargets.insert(bountyTargets[index]->getUID());
								bountyTargets.erase(bountyTargets.begin() + index);
							}
						}
					}
				}
			}
		}
	}
	updateClientBounties(true);
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
			messagePlayer(0, MESSAGE_DEBUG, "Uid: %d, achievement: %d, ticks: %d, counter: %d", (*it).first, (*inner_it).first, (*inner_it).second.first, (*inner_it).second.second);
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

std::map<ItemType, Uint32> dapperItems;
std::set<ItemType> AchievementObserver::PlayerAchievements::startingClassItems =
{
	LEATHER_HELM,
	HAT_PHRYGIAN,
	HAT_HOOD_WHISPERS,
	HAT_WIZARD,
	HAT_HOOD_APPRENTICE,
	HAT_JESTER,
	HAT_FEZ,
	HAT_HOOD_ASSASSIN,
	MONOCLE,
	MASK_BANDIT,
	PUNISHER_HOOD,
	MASK_SHAMAN,
	TOOL_BLINDFOLD_TELEPATHY,
	TOOL_BLINDFOLD
};

int AchievementObserver::PlayerAchievements::getItemIndexForDapperAchievement(Item* item)
{
	if ( dapperItems.empty() )
	{
		int index = 0;
		for ( int i = 0; i < NUMITEMS; ++i )
		{
			if ( startingClassItems.find((ItemType)i) != startingClassItems.end() )
			{
				continue;
			}
			if ( items[i].item_slot == EQUIPPABLE_IN_SLOT_HELM )
			{
				dapperItems[(ItemType)i] = index;
				++index;
			}
			else if ( items[i].item_slot == EQUIPPABLE_IN_SLOT_MASK )
			{
				dapperItems[(ItemType)i] = index;
				++index;
			}
		}
	}

	if ( item )
	{
		auto find = dapperItems.find(item->type);
		if ( find != dapperItems.end() )
		{
			return find->second;
		}
	}
	return -1;
}

void AchievementObserver::updatePlayerAchievement(int player, Achievement achievement, AchievementEvent achEvent)
{
	switch ( achievement )
	{
		case BARONY_ACH_BY_THE_BOOK:
			if ( player == clientnum )
			{
				if ( achEvent == BY_THE_BOOK_COMPENDIUM_PAGE )
				{
					playerAchievements[player].ticksByTheBookViewed = ticks;
				}
				else if ( achEvent == BY_THE_BOOK_BREW )
				{
					if ( playerAchievements[player].ticksByTheBookViewed != 0
						&& ticks > playerAchievements[player].ticksByTheBookViewed )
					{
						if ( (ticks - playerAchievements[player].ticksByTheBookViewed) < TICKS_PER_SECOND * 10 )
						{
							awardAchievement(player, achievement);
						}
					}
				}
			}
			break;
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
						if ( !(g_SteamStats[statBitCounter].m_iValue & (1 << (levelnum - 1))) ) // bit not set
						{
							// update with the difference in values.
							steamStatisticUpdate(statBitCounter, STEAM_STAT_INT, (1 << (levelnum - 1)));
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
		case BARONY_ACH_DAPPER:
		{

			int loops = 2;
			SteamStatIndexes statLevelTotal = STEAM_STAT_DAPPER;
			while ( loops > 0 )
			{
				--loops;
				int index = playerAchievements[player].getItemIndexForDapperAchievement(loops == 1 ? stats[player]->helmet : stats[player]->mask);
				if ( index >= 0 )
				{
					SteamStatIndexes statBitCounter = STEAM_STAT_DAPPER_1;
					if ( index >= 32 )
					{
						statBitCounter = STEAM_STAT_DAPPER_2;
					}
					else if ( index >= 64 )
					{
						statBitCounter = STEAM_STAT_DAPPER_3;
					}
				
					Uint32 bit = (1 << (index % 32));
					if ( !(g_SteamStats[statBitCounter].m_iValue & (bit)) ) // bit not set
					{
						// update with the difference in values.
						steamStatisticUpdate(statBitCounter, STEAM_STAT_INT, (bit));
					}
				}
			}

			if ( ticks % (TICKS_PER_SECOND * 5) == 0 )
			{
				int hatmasksWorn = 0;
				for ( int i = 0; i < 32; ++i )
				{
					if ( g_SteamStats[STEAM_STAT_DAPPER_1].m_iValue & (1 << i) ) // count the bits
					{
						++hatmasksWorn;
					}
					if ( g_SteamStats[STEAM_STAT_DAPPER_2].m_iValue & (1 << i) ) // count the bits
					{
						++hatmasksWorn;
					}
					if ( g_SteamStats[STEAM_STAT_DAPPER_3].m_iValue & (1 << i) ) // count the bits
					{
						++hatmasksWorn;
					}
				}
				if ( hatmasksWorn >= g_SteamStats[statLevelTotal].m_iValue )
				{
					steamStatisticUpdate(statLevelTotal, STEAM_STAT_INT, hatmasksWorn - g_SteamStats[statLevelTotal].m_iValue);
				}
			}
			break;
		}
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
			std::vector<int> awardAchievementsToAllPlayers;
			int num = 0;
			for ( int i = 0; i < MAXPLAYERS; ++i )
			{
				if ( !client_disconnected[i] )
				{
					if ( stats[i] && stats[i]->playerRace != RACE_HUMAN && stats[i]->stat_appearance == 0 )
					{
						races.insert(stats[i]->playerRace);
					}
					if ( client_classes[i] > CLASS_MONK )
					{
						classes.insert(client_classes[i]);
					}
					++num;
				}
			}
			if ( gameModeManager.currentSession.challengeRun.isActive() && num >= 2 )
			{
				awardAchievementsToAllPlayers.push_back(BARONY_ACH_SPROUTS);
			}
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
	//messagePlayer(player, MESSAGE_DEBUG, "[DEBUG]: Processed achievement %d, event: %d", achievement, achEvent);
#endif
}

bool AchievementObserver::PlayerAchievements::allPlayersDeadEvent = false;

void AchievementObserver::clearPlayerAchievementData()
{
	PlayerAchievements::allPlayersDeadEvent = false;
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
		playerAchievements[i].phantomMaskFirstStrikes.clear();
		playerAchievements[i].bountyTargets.clear();
		playerAchievements[i].updatedBountyTargets = false;
		playerAchievements[i].wearingBountyHat = false;
		playerAchievements[i].totalKillsTickUpdate = false;
		playerAchievements[i].ticksByTheBookViewed = 0;
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
		case BARONY_ACH_SPROUTS:
			steamAchievementClient(player, "BARONY_ACH_SPROUTS");
			break;
		case BARONY_ACH_BY_THE_BOOK:
			steamAchievementClient(player, "BARONY_ACH_BY_THE_BOOK");
			break;
		default:
			messagePlayer(player, MESSAGE_DEBUG, "[WARNING]: Unhandled achievement: %d", achievement);
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
		(int)floor(target->x / 16), (int)floor(target->y / 16), player, target, GeneratePathTypes::GENERATE_PATH_ACHIEVEMENT, true);
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

void AchievementObserver::updateGlobalStat(int index, int player)
{
	if ( multiplayer == CLIENT )
	{
		return;
	}
#ifndef DEBUG_ACHIEVEMENTS
	if ( conductGameChallenges[CONDUCT_CHEATS_ENABLED]
		|| conductGameChallenges[CONDUCT_MODDED_NO_ACHIEVEMENTS]
		|| Mods::disableSteamAchievements )
	{
		return;
	}
#endif
#ifdef USE_PLAYFAB
	playfabUser.globalStat(index, player);
#endif
#if defined USE_EOS
	EOS.queueGlobalStatUpdate(index, 1);
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
		case BAT_SMALL:
			return STEAM_GSTAT_DEATHS_BAT;
		case BUGBEAR:
			return STEAM_GSTAT_DEATHS_BUGBEAR;
		case MIMIC:
			return STEAM_GSTAT_DEATHS_MIMIC;
		default:
			return STEAM_GSTAT_INVALID;
	}
	return STEAM_GSTAT_INVALID;
}

void SaveGameInfo::computeHash(const int playernum, Uint32& hash)
{
	if ( players.size() <= playernum ) 
	{
		return;
	}

	Uint32 shift = 0;
	hash += (Uint32)((Uint32)gamekey << (shift % 32)); ++shift;
	hash += (Uint32)((Uint32)mapseed << (shift % 32)); ++shift;
	hash += (Uint32)((Uint32)gametimer << (shift % 32)); ++shift;
	hash += (Uint32)((Uint32)svflags << (shift % 32)); ++shift;
	hash += (Uint32)((Uint32)multiplayer_type << (shift % 32)); ++shift;
	hash += (Uint32)((Uint32)dungeon_lvl << (shift % 32)); ++shift;
	hash += (Uint32)((Uint32)level_track << (shift % 32)); ++shift;
	hash += (Uint32)((Uint32)customseed << (shift % 32)); ++shift;

	auto& player = players[playernum];
	hash += (Uint32)((Uint32)player.char_class << (shift % 32)); ++shift;
	hash += (Uint32)((Uint32)player.race << (shift % 32)); ++shift;

	for ( auto k : player.kills )
	{
		hash += (Uint32)((Uint32)k << (shift % 32)); ++shift;
	}

	hash += (Uint32)((Uint32)player.conductPenniless << (shift % 32)); ++shift;
	hash += (Uint32)((Uint32)player.conductFoodless << (shift % 32)); ++shift;
	hash += (Uint32)((Uint32)player.conductVegetarian << (shift % 32)); ++shift;
	hash += (Uint32)((Uint32)player.conductIlliterate << (shift % 32)); ++shift;
	for ( int i = 0; i < NUM_CONDUCT_CHALLENGES; ++i )
	{
		hash += (Uint32)((Uint32)player.additionalConducts[i] << (shift % 32)); ++shift;
	}
	for ( int i = 0; i < NUM_GAMEPLAY_STATISTICS; ++i )
	{
		hash += (Uint32)((Uint32)player.gameStatistics[i] << (shift % 32)); ++shift;
	}
	for ( int i = 0; i < NUM_HOTBAR_SLOTS; ++i )
	{
		hash += (Uint32)((Uint32)player.hotbar[i] << (shift % 32)); ++shift;
		for ( int j = 0; j < NUM_HOTBAR_ALTERNATES; ++j )
		{
			hash += (Uint32)((Uint32)player.hotbar_alternate[j][i] << (shift % 32)); ++shift;
		}
	}
	for ( auto k : player.spells )
	{
		hash += (Uint32)((Uint32)k << (shift % 32)); ++shift;
	}

	std::vector<Player::stat_t*> statsArr;
	statsArr.push_back(&players[playernum].stats);
	for ( auto& s : players[playernum].followers )
	{
		statsArr.push_back(&s);
	}

	for ( auto stats : statsArr )
	{
		hash += (Uint32)((Uint32)stats->type << (shift % 32)); ++shift;
		hash += (Uint32)((Uint32)stats->sex << (shift % 32)); ++shift;
		hash += (Uint32)((Uint32)stats->statscore_appearance << (shift % 32)); ++shift;

		hash += (Uint32)((Uint32)stats->HP << (shift % 32)); ++shift;
		hash += (Uint32)((Uint32)stats->maxHP << (shift % 32)); ++shift;
		hash += (Uint32)((Uint32)stats->MP << (shift % 32)); ++shift;
		hash += (Uint32)((Uint32)stats->maxMP << (shift % 32)); ++shift;
		hash += (Uint32)((Uint32)stats->STR << (shift % 32)); ++shift;
		hash += (Uint32)((Uint32)stats->DEX << (shift % 32)); ++shift;
		hash += (Uint32)((Uint32)stats->CON << (shift % 32)); ++shift;
		hash += (Uint32)((Uint32)stats->INT << (shift % 32)); ++shift;
		hash += (Uint32)((Uint32)stats->PER << (shift % 32)); ++shift;
		hash += (Uint32)((Uint32)stats->CHR << (shift % 32)); ++shift;
		hash += (Uint32)((Uint32)stats->EXP << (shift % 32)); ++shift;
		hash += (Uint32)((Uint32)stats->LVL << (shift % 32)); ++shift;
		hash += (Uint32)((Uint32)stats->GOLD << (shift % 32)); ++shift;
		hash += (Uint32)((Uint32)stats->HUNGER << (shift % 32)); ++shift;

		for ( auto k : stats->PROFICIENCIES )
		{
			hash += (Uint32)((Uint32)k << (shift % 32)); ++shift;
		}
		for ( auto k : stats->EFFECTS )
		{
			hash += (Uint32)((Uint32)k << (shift % 32)); ++shift;
		}
		for ( auto k : stats->EFFECTS_TIMERS )
		{
			hash += (Uint32)((Uint32)k << (shift % 32)); ++shift;
		}
		for ( auto k : stats->MISC_FLAGS )
		{
			hash += (Uint32)((Uint32)k << (shift % 32)); ++shift;
		}
		for ( auto& pair : stats->player_equipment )
		{
			hash += (Uint32)((Uint32)pair.second << (shift % 32)); ++shift;
		}
		for ( auto& pair : stats->npc_equipment )
		{
			pair.second.computeHash(hash, shift);
		}
		for ( auto& item : stats->inventory )
		{
			item.computeHash(hash, shift);
		}
		for ( auto& bag : stats->player_lootbags )
		{
			hash += (Uint32)((Uint32)bag.first << (shift % 32)); ++shift;
			hash += (Uint32)((Uint32)bag.second.spawn_x << (shift % 32)); ++shift;
			hash += (Uint32)((Uint32)bag.second.spawn_y << (shift % 32)); ++shift;
			hash += (Uint32)((Uint32)bag.second.looted << (shift % 32)); ++shift;
			hash += (Uint32)((Uint32)bag.second.spawnedOnGround << (shift % 32)); ++shift;
			for ( auto& item : bag.second.items )
			{
				item.computeHash(hash, shift);
			}
		}
	}

	for ( auto& pair : additional_data )
	{
		hash += djb2Hash(const_cast<char*>(pair.first.c_str()));
		hash += djb2Hash(const_cast<char*>(pair.second.c_str()));
	}
}

void SaveGameInfo::Player::stat_t::item_t::computeHash(Uint32& hash, Uint32& shift)
{
	hash += (Uint32)((Uint32)type << (shift % 32)); ++shift;
	hash += (Uint32)((Uint32)status << (shift % 32)); ++shift;
	hash += (Uint32)((Uint32)appearance << (shift % 32)); ++shift;
	hash += (Uint32)((Uint32)beatitude << (shift % 32)); ++shift;
	hash += (Uint32)((Uint32)count << (shift % 32)); ++shift;
	hash += (Uint32)((Uint32)identified << (shift % 32)); ++shift;
	hash += (Uint32)((Uint32)x << (shift % 32)); ++shift;
	hash += (Uint32)((Uint32)y << (shift % 32)); ++shift;
}

int SaveGameInfo::populateFromSession(const int playernum)
{
	auto info = this;

	auto t = getTime();
	struct tm* tm = localtime(&t); assert(tm);

	// save info
	char buf[64];
	info->game_version = getSavegameVersion(VERSION);
	info->timestamp = getTimeAndDateFormatted(t, buf, sizeof(buf));

	// savefile hash
	info->hash = tm->tm_hour + tm->tm_mday * tm->tm_year + tm->tm_wday + tm->tm_yday;
	if ( info->game_version < 410 )
	{
		info->hash += stats[playernum]->STR + stats[playernum]->LVL + stats[playernum]->DEX * stats[playernum]->INT;
		info->hash += stats[playernum]->CON * stats[playernum]->PER + std::min(stats[playernum]->GOLD, 5000) - stats[playernum]->CON;
		info->hash += stats[playernum]->HP - stats[playernum]->MP;
		info->hash += currentlevel;
	}

	// game info
	info->gamename = stats[playernum]->name;
	info->gamekey = ::uniqueGameKey;
	info->lobbykey = ::uniqueLobbyKey;
	info->mapseed = ::mapseed;
	info->customseed = gameModeManager.currentSession.seededRun.seed;
	info->customseed_string = gameModeManager.currentSession.seededRun.seedString;
	info->gametimer = completionTime;
	info->svflags = svFlags;
	info->player_num = playernum;
	info->hiscore_killed_by = stats[playernum]->killer;
	info->hiscore_killed_item = stats[playernum]->killer_item;
	info->hiscore_killed_monster = stats[playernum]->killer_monster;

	// multiplayer type
	if ( multiplayer == SINGLE ) {
		info->multiplayer_type = splitscreen ? SPLITSCREEN : SINGLE;
	}
	else {
		if ( directConnect ) {
			info->multiplayer_type = multiplayer == SERVER ? DIRECTSERVER : DIRECTCLIENT;
		}
		else {
			if ( multiplayer == SERVER ) {
				info->multiplayer_type = LobbyHandler.hostingType == LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY ?
					SERVERCROSSPLAY : SERVER;
			}
			else if ( multiplayer == CLIENT ) {
				info->multiplayer_type = CLIENT;
			}
			else {
				printlog("saveGame(): failed to save, unknown game type!");
				return 1;
			}
		}
	}

	info->dungeon_lvl = currentlevel;
	info->level_track = secretlevel ? 1 : 0;
	info->players_connected.resize(MAXPLAYERS);
	info->players.resize(MAXPLAYERS);
	for ( int c = 0; c < MAXPLAYERS; ++c ) {
		info->players_connected[c] = client_disconnected[c] ? 0 : 1;
		if ( info->players_connected[c] ) {
			auto& player = info->players[c];
			player.char_class = client_classes[c];
			player.race = stats[c]->playerRace;

			// the following player info is shared by all players currently
			player.kills.resize(NUMMONSTERS);
			for ( int i = 0; i < NUMMONSTERS; ++i ) {
				player.kills[i] = kills[i];
			}
			player.conductPenniless = conductPenniless;
			player.conductFoodless = conductFoodless;
			player.conductVegetarian = conductVegetarian;
			player.conductIlliterate = conductIlliterate;
			for ( int i = 0; i < NUM_CONDUCT_CHALLENGES; ++i ) {
				player.additionalConducts[i] = conductGameChallenges[i];
			}
			for ( int i = 0; i < NUM_GAMEPLAY_STATISTICS; ++i ) {
				player.gameStatistics[i] = gameStatistics[i];
			}

			// hotbar
			if ( ::players[c]->isLocalPlayer() )
			{
				spell_t* oldSelectedSpell = ::players[c]->magic.selectedSpell();
				auto lastSelectedSpellAppearance = ::players[c]->magic.selected_spell_last_appearance;

				bool reinitShapeshiftHotbar = false;
				if ( ::players[c]->hotbar.swapHotbarOnShapeshift > 0 )
				{
					reinitShapeshiftHotbar = true;
					deinitShapeshiftHotbar(c);
				}
				for ( int i = 0; i < NUM_HOTBAR_SLOTS; ++i ) {
					auto item = uidToItem(::players[c]->hotbar.slots()[i].item);
					if ( item ) {
						player.hotbar[i] = list_Index(item->node);
					}
					else {
						player.hotbar[i] = UINT32_MAX;
					}

					for ( int j = 0; j < NUM_HOTBAR_ALTERNATES; ++j ) {
						auto item = uidToItem(::players[c]->hotbar.slotsAlternate()[j][i].item);
						if ( item )
						{
							player.hotbar_alternate[j][i] = list_Index(item->node);
						}
						else
						{
							player.hotbar_alternate[j][i] = UINT32_MAX;
						}
					}
				}


				// spells
				player.selected_spell_last_appearance = lastSelectedSpellAppearance;
				player.selected_spell = UINT32_MAX;
				for ( int i = 0; i < NUM_HOTBAR_ALTERNATES; ++i )
				{
					player.selected_spell_alternate[i] = UINT32_MAX;
				}

				for ( node_t* node = ::players[c]->magic.spellList.first;
					node != nullptr; node = node->next ) {
					auto spell = (spell_t*)node->element;
					player.spells.push_back(spell->ID);

					if ( ::players[c]->magic.selectedSpell() == spell )
					{
						player.selected_spell = list_Index(node);
					}
					for ( int i = 0; i < NUM_HOTBAR_ALTERNATES; ++i )
					{
						if ( ::players[c]->magic.selected_spell_alternate[i] == spell )
						{
							player.selected_spell_alternate[i] = list_Index(node);
						}
					}
				}

				if ( reinitShapeshiftHotbar )
				{
					initShapeshiftHotbar(c);
				}

				::players[c]->magic.equipSpell(oldSelectedSpell);
				::players[c]->magic.selected_spell_last_appearance = lastSelectedSpellAppearance;
			}
			else
			{
				for ( int i = 0; i < NUM_HOTBAR_SLOTS; ++i ) {
					player.hotbar[i] = UINT32_MAX;
					for ( int j = 0; j < NUM_HOTBAR_ALTERNATES; ++j ) {
						player.hotbar_alternate[j][i] = UINT32_MAX;
					}
				}

				// spells
				player.selected_spell = UINT32_MAX;
				player.selected_spell_last_appearance = -1;
				for ( int i = 0; i < NUM_HOTBAR_ALTERNATES; ++i )
				{
					player.selected_spell_alternate[i] = UINT32_MAX;
				}
			}

			// known alchemy recipes and known scrolls
			player.known_recipes = clientLearnedAlchemyRecipes[c];
			for ( auto& entry : clientLearnedScrollLabels[c] ) {
				player.known_scrolls.push_back(entry);
			}

			// player stats
			player.stats.name = stats[c]->name;
			player.stats.type = stats[c]->type;
			player.stats.sex = stats[c]->sex;
			player.stats.statscore_appearance = stats[c]->stat_appearance;
			player.stats.HP = stats[c]->HP;
			player.stats.maxHP = stats[c]->MAXHP;
			player.stats.MP = stats[c]->MP;
			player.stats.maxMP = stats[c]->MAXMP;
			player.stats.STR = stats[c]->STR;
			player.stats.DEX = stats[c]->DEX;
			player.stats.CON = stats[c]->CON;
			player.stats.INT = stats[c]->INT;
			player.stats.PER = stats[c]->PER;
			player.stats.CHR = stats[c]->CHR;
			player.stats.EXP = stats[c]->EXP;
			player.stats.LVL = stats[c]->LVL;
			player.stats.GOLD = stats[c]->GOLD;
			player.stats.HUNGER = stats[c]->HUNGER;
			player.stats.PROFICIENCIES.resize(NUMPROFICIENCIES);
			for ( int i = 0; i < NUMPROFICIENCIES; ++i ) {
				player.stats.PROFICIENCIES[i] = stats[c]->getProficiency(i);
			}
			player.stats.EFFECTS.resize(NUMEFFECTS);
			player.stats.EFFECTS_TIMERS.resize(NUMEFFECTS);
			for ( int i = 0; i < NUMEFFECTS; ++i ) {
				player.stats.EFFECTS[i] = stats[c]->EFFECTS[i];
				player.stats.EFFECTS_TIMERS[i] = stats[c]->EFFECTS_TIMERS[i];
			}
			constexpr int NUMMISCFLAGS = sizeof(Stat::MISC_FLAGS) / sizeof(Stat::MISC_FLAGS[0]);
			player.stats.MISC_FLAGS.resize(NUMMISCFLAGS);
			for ( int i = 0; i < NUMMISCFLAGS; ++i ) {
				player.stats.MISC_FLAGS[i] = stats[c]->MISC_FLAGS[i];
			}
			//player.stats.attributes = ; // players have no key/value table
			for ( auto& h : ShopkeeperPlayerHostility.playerHostility[c] )
			{
				player.shopkeeperHostility.push_back(std::make_pair(h.first, SaveGameInfo::Player::PlayerRaceHostility_t()));
				auto& h2 = player.shopkeeperHostility.at(player.shopkeeperHostility.size() - 1);
				h2.second.wantedLevel = h.second.wantedLevel;
				h2.second.playerRace = h.second.playerRace;
				h2.second.sex = h.second.sex;
				h2.second.equipment = h.second.equipment;
				h2.second.type = h.second.type;
				h2.second.player = h.second.player;
				h2.second.numKills = h.second.numKills;
				h2.second.numAggressions = h.second.numAggressions;
				h2.second.numAccessories = h.second.numAccessories;
			}
			for ( auto& pair : ::players[c]->mechanics.itemDegradeRng )
			{
				player.itemDegradeRNG.push_back(pair);
			}
			player.sustainedSpellMPUsed = ::players[c]->mechanics.sustainedSpellMPUsed;

			for ( auto& pair : ::players[c]->compendiumProgress.itemEvents )
			{
				player.compendium_item_events.push_back(std::make_pair(pair.first, std::vector<int>()));
				auto& vec_entry = player.compendium_item_events.back();
				for ( auto& itemValue : pair.second )
				{
					vec_entry.second.push_back(itemValue.first);
					vec_entry.second.push_back(itemValue.second);
				}
			}

			for ( auto& loot : stats[c]->player_lootbags )
			{
				player.stats.player_lootbags.push_back(std::make_pair(loot.first,
					SaveGameInfo::Player::stat_t::lootbag_t(
						loot.second.spawn_x,
						loot.second.spawn_y,
						loot.second.spawnedOnGround,
						loot.second.looted
					)));
				auto& loot2 = player.stats.player_lootbags.at(player.stats.player_lootbags.size() - 1);
				for ( auto& item : loot.second.items )
				{
					loot2.second.items.push_back(SaveGameInfo::Player::stat_t::item_t(
						(Uint32)item.type,
						(Uint32)item.status,
						item.appearance,
						item.beatitude,
						item.count,
						item.identified,
						0,
						0));
				}
			}

			// equipment slots
			const std::vector<std::pair<std::string, Item*>> player_slots = {
				{"helmet", stats[c]->helmet},
				{"breastplate", stats[c]->breastplate},
				{"gloves", stats[c]->gloves},
				{"shoes", stats[c]->shoes},
				{"shield", stats[c]->shield},
				{"weapon", stats[c]->weapon},
				{"cloak", stats[c]->cloak},
				{"amulet", stats[c]->amulet},
				{"ring", stats[c]->ring},
				{"mask", stats[c]->mask},
			};
			if ( ::players[c]->isLocalPlayer() ) {
				// if this is a local player, we have their inventory, and can store
				// item indexes in the player_equipment table
				for ( auto& slot : player_slots ) {
					player.stats.player_equipment.push_back(std::make_pair(slot.first,
						slot.second ? list_Index(slot.second->node) : UINT32_MAX));
				}
			}
			else {
				// if this is not a local player, we don't have the inventory.
				// we must save whole items for each slot which the host will
				// restore later
				for ( auto& slot : player_slots ) {
					if ( slot.second ) {
						player.stats.npc_equipment.push_back(std::make_pair(
							slot.first, SaveGameInfo::Player::stat_t::item_t{
								(Uint32)slot.second->type,
								(Uint32)slot.second->status,
								slot.second->appearance,
								slot.second->beatitude,
								slot.second->count,
								slot.second->identified,
								slot.second->x,
								slot.second->y,
							}));
					}
				}
			}

			// inventory
			for ( node_t* node = stats[c]->inventory.first;
				node != nullptr; node = node->next ) {
				auto item = (Item*)node->element;
				player.stats.inventory.push_back(
					SaveGameInfo::Player::stat_t::item_t{
					(Uint32)item->type,
					(Uint32)item->status,
					item->appearance,
					item->beatitude,
					item->count,
					item->identified,
					item->x,
					item->y,
					});
			}

			// followers
			for ( node_t* node = stats[c]->FOLLOWERS.first;
				node != nullptr; node = node->next ) {
				auto entity = uidToEntity(*((Uint32*)node->element));
				Stat* follower = entity ? entity->getStats() : nullptr;
				if ( follower ) {
					SaveGameInfo::Player::stat_t stats;
					stats.name = follower->name;
					stats.type = follower->type;
					stats.sex = follower->sex;
					stats.statscore_appearance = follower->stat_appearance;
					stats.HP = follower->HP;
					stats.maxHP = follower->MAXHP;
					stats.MP = follower->MP;
					stats.maxMP = follower->MAXMP;
					stats.STR = follower->STR;
					stats.DEX = follower->DEX;
					stats.CON = follower->CON;
					stats.INT = follower->INT;
					stats.PER = follower->PER;
					stats.CHR = follower->CHR;
					stats.EXP = follower->EXP;
					stats.LVL = follower->LVL;
					stats.GOLD = follower->GOLD;
					stats.HUNGER = follower->HUNGER;
					stats.PROFICIENCIES.resize(NUMPROFICIENCIES);
					for ( int i = 0; i < NUMPROFICIENCIES; ++i ) {
						stats.PROFICIENCIES[i] = follower->getProficiency(i);
					}
					stats.EFFECTS.resize(NUMEFFECTS);
					stats.EFFECTS_TIMERS.resize(NUMEFFECTS);
					for ( int i = 0; i < NUMEFFECTS; ++i ) {
						stats.EFFECTS[i] = follower->EFFECTS[i];
						stats.EFFECTS_TIMERS[i] = follower->EFFECTS_TIMERS[i];
					}
					stats.MISC_FLAGS.resize(NUMMISCFLAGS);
					for ( int i = 0; i < NUMMISCFLAGS; ++i ) {
						stats.MISC_FLAGS[i] = follower->MISC_FLAGS[i];
					}
					for ( auto& attribute : follower->attributes ) {
						stats.attributes.push_back(attribute);
					}

					// equipment slots
					const std::vector<std::pair<std::string, Item*>> npc_slots = {
						{"helmet", follower->helmet},
						{"breastplate", follower->breastplate},
						{"gloves", follower->gloves},
						{"shoes", follower->shoes},
						{"shield", follower->shield},
						{"weapon", follower->weapon},
						{"cloak", follower->cloak},
						{"amulet", follower->amulet},
						{"ring", follower->ring},
						{"mask", follower->mask},
					};
					for ( auto& slot : npc_slots ) {
						if ( slot.second ) {
							stats.npc_equipment.push_back(std::make_pair(
								slot.first, SaveGameInfo::Player::stat_t::item_t{
									(Uint32)slot.second->type,
									(Uint32)slot.second->status,
									slot.second->appearance,
									slot.second->beatitude,
									slot.second->count,
									slot.second->identified,
									slot.second->x,
									slot.second->y,
								}));
						}
					}

					// inventory
					for ( node_t* node = follower->inventory.first;
						node != nullptr; node = node->next ) {
						auto item = (Item*)node->element;
						stats.inventory.push_back(
							SaveGameInfo::Player::stat_t::item_t{
							(Uint32)item->type,
							(Uint32)item->status,
							item->appearance,
							item->beatitude,
							item->count,
							item->identified,
							item->x,
							item->y,
							});
					}

					// finally, add follower to list
					player.followers.push_back(stats);
				}
			}
		}
	}

	info->map_messages = ::Player::Minimap_t::mapDetails;
	if ( gameModeManager.currentSession.challengeRun.isActive() )
	{
		info->additional_data.push_back(std::make_pair("game_scenario", gameModeManager.currentSession.challengeRun.scenarioStr));
	}


	if ( info->game_version >= 410 )
	{
		info->computeHash(info->player_num, info->hash);
	}
	return 0;
}

int saveGame(int saveIndex) {
	if (!gameModeManager.allowsSaves()) {
		return 1; // can't save tutorial games
	}
	if (!intro) {
		messagePlayer(clientnum, MESSAGE_MISC, Language::get(1121));
	}
	
	SaveGameInfo info;
	if ( info.populateFromSession(clientnum) != 0 )
	{
		return 1;
	}

	static ConsoleVariable<bool> cvar_saveText("/save_text_format", true);

	char path[PATH_MAX] = "";
	std::string savefile = setSaveGameFileName(multiplayer == SINGLE, SaveFileType::JSON, saveIndex);
	completePath(path, savefile.c_str(), outputdir);
	auto result = FileHelper::writeObject(path, *cvar_saveText ? EFileFormat::Json : EFileFormat::Binary, info);
	return result == true ? 0 : 1;
}

int SaveGameInfo::getTotalScore(const int playernum, const int victory)
{
	auto player = players[playernum];
	Player::stat_t* stats = &players[playernum].stats;
	int amount = 0;

	for ( auto& item : stats->inventory )
	{
		amount += items[item.type].value;
	}
	amount += stats->GOLD;
	amount += stats->EXP;
	amount += stats->LVL * 500;

	for ( int c = 0; c < NUMPROFICIENCIES; c++ )
	{
		amount += stats->PROFICIENCIES[c];
	}
	for ( int c = 0; c < NUMMONSTERS; c++ )
	{
		if ( c != HUMAN )
		{
			amount += player.kills[c] * 100;
		}
		else
		{
			amount -= player.kills[c] * 100;
		}
	}

	amount += this->dungeon_lvl * 500;
	if ( victory == 100 )
	{
		amount += 2 * 20000;
	}
	else if ( victory == 101 )
	{
		amount += 5 * 20000;
	}
	else if ( victory >= 3 )
	{
		amount += 3 * 20000;
	}
	else
	{
		amount += victory * 10000;
	}

	Uint32 gametimer = std::min(this->gametimer, (Uint32)0xFFFFFF);

	amount -= gametimer / TICKS_PER_SECOND;
	if ( victory )
	{
		amount += player.conductPenniless * 5000;
		amount += player.conductFoodless * 5000;
		amount += player.conductVegetarian * 5000;
		amount += player.conductIlliterate * 5000;
		amount += player.additionalConducts[CONDUCT_BOOTS_SPEED] * 20000;
		amount += player.additionalConducts[CONDUCT_BRAWLER] * 20000;
		amount += player.additionalConducts[CONDUCT_RANGED_ONLY] * 20000;
		amount += player.additionalConducts[CONDUCT_ACCURSED] * 50000;
		amount += player.additionalConducts[CONDUCT_BLESSED_BOOTS_SPEED] * 100000;
		if ( player.additionalConducts[CONDUCT_HARDCORE] == 1
			&& player.additionalConducts[CONDUCT_CHEATS_ENABLED] == 0 )
		{
			amount *= 2;
		}
		if ( player.additionalConducts[CONDUCT_KEEPINVENTORY] &&
			player.additionalConducts[CONDUCT_MULTIPLAYER] )
		{
			amount /= 2;
		}
		if ( player.additionalConducts[CONDUCT_LIFESAVING] )
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

std::string SaveGameInfo::serializeToOnlineHiscore(const int playernum, const int victory)
{
	rapidjson::Document d;
	d.SetObject();

	rapidjson::Value character(rapidjson::kObjectType);

	auto& player = players[playernum];
	auto& myStats = players[playernum].stats;

	Uint32 gametimer = std::min(this->gametimer, (Uint32)0xFFFFFF);

	std::string lid = "lid";
	int lid_version = 0;
	int challengeEventSave = 0;
	for ( auto& pair : additional_data )
	{
		if ( pair.first == "game_scenario" )
		{
			GameModeManager_t::CurrentSession_t::ChallengeRun_t run;
			run.setup(pair.second);
			if ( run.isActive() )
			{
				lid = run.lid;
				lid_version = run.lid_version;
			}
			break;
		}
	}

	character.AddMember("version", rapidjson::Value(1), d.GetAllocator());
	character.AddMember("game_ver", rapidjson::Value(VERSION, d.GetAllocator()), d.GetAllocator());
	character.AddMember("leaderboard", rapidjson::Value(lid.c_str(), d.GetAllocator()), d.GetAllocator());
	character.AddMember("leaderboard_version", rapidjson::Value(lid_version), d.GetAllocator());
	character.AddMember("time", rapidjson::Value(gametimer), d.GetAllocator());
	character.AddMember("totalscore", rapidjson::Value(getTotalScore(playernum, victory)), d.GetAllocator());
	character.AddMember("seed", rapidjson::Value(customseed), d.GetAllocator());
	character.AddMember("seed_str", rapidjson::Value(customseed_string.c_str(), d.GetAllocator()), d.GetAllocator());

	character.AddMember("victory", rapidjson::Value(victory), d.GetAllocator());
	int multi = multiplayer;
	if ( multi == 0 )
	{
		if ( player.additionalConducts[CONDUCT_MULTIPLAYER] )
		{
			multi = CLIENT; // failsafe to set if session wrapped up prematurely
		}
	}
	character.AddMember("multiplayer", rapidjson::Value(multi), d.GetAllocator());
	character.AddMember("splitscreen", rapidjson::Value(splitscreen), d.GetAllocator());
	character.AddMember("flags", rapidjson::Value(svflags), d.GetAllocator());
	character.AddMember("lvl", rapidjson::Value(dungeon_lvl), d.GetAllocator());
	character.AddMember("secret", rapidjson::Value(level_track), d.GetAllocator());

	{
		rapidjson::Value statsObj(rapidjson::kObjectType);
		//statsObj.AddMember("name", rapidjson::Value(myStats.name.c_str(), d.GetAllocator()), d.GetAllocator());
		statsObj.AddMember("MAXHP", myStats.maxHP, d.GetAllocator());
		statsObj.AddMember("MAXMP", myStats.maxMP, d.GetAllocator());
		statsObj.AddMember("STR", myStats.STR, d.GetAllocator());
		statsObj.AddMember("DEX", myStats.DEX, d.GetAllocator());
		statsObj.AddMember("CON", myStats.CON, d.GetAllocator());
		statsObj.AddMember("INT", myStats.INT, d.GetAllocator());
		statsObj.AddMember("PER", myStats.PER, d.GetAllocator());
		statsObj.AddMember("CHR", myStats.CHR, d.GetAllocator());

		statsObj.AddMember("LVL", myStats.LVL, d.GetAllocator());
		statsObj.AddMember("EXP", myStats.EXP, d.GetAllocator());
		statsObj.AddMember("race", player.race, d.GetAllocator());
		statsObj.AddMember("appearance", myStats.statscore_appearance, d.GetAllocator());
		statsObj.AddMember("sex", myStats.sex, d.GetAllocator());
		statsObj.AddMember("class", player.char_class, d.GetAllocator());

		statsObj.AddMember("GOLD", myStats.GOLD, d.GetAllocator());
		statsObj.AddMember("kill_by", hiscore_killed_by, d.GetAllocator());
		statsObj.AddMember("kill_mon", hiscore_killed_monster, d.GetAllocator());
		statsObj.AddMember("kill_item", hiscore_killed_item, d.GetAllocator());

		character.AddMember("stats", statsObj, d.GetAllocator());

		rapidjson::Value attrObj(rapidjson::kObjectType);
		rapidjson::Value killsArr(rapidjson::kArrayType);
		for ( int i = 0; i < NUMMONSTERS; ++i )
		{
			killsArr.PushBack(player.kills[i], d.GetAllocator());
		}
		attrObj.AddMember("kills", killsArr, d.GetAllocator());

		rapidjson::Value profArr(rapidjson::kArrayType);
		for ( int i = 0; i < NUMPROFICIENCIES; ++i )
		{
			profArr.PushBack(myStats.PROFICIENCIES[i], d.GetAllocator());
		}
		attrObj.AddMember("proficiencies", profArr, d.GetAllocator());

		rapidjson::Value conductsArr(rapidjson::kArrayType);
		conductsArr.PushBack((int)player.conductPenniless, d.GetAllocator());
		conductsArr.PushBack((int)player.conductFoodless, d.GetAllocator());
		conductsArr.PushBack((int)player.conductVegetarian, d.GetAllocator());
		conductsArr.PushBack((int)player.conductIlliterate, d.GetAllocator());
		for ( int i = 0; i < NUM_CONDUCT_CHALLENGES; ++i )
		{
			conductsArr.PushBack(player.additionalConducts[i], d.GetAllocator());
		}
		attrObj.AddMember("conducts", conductsArr, d.GetAllocator());

		rapidjson::Value statisticsArr(rapidjson::kArrayType);
		for ( int i = 0; i < NUM_GAMEPLAY_STATISTICS; ++i )
		{
			statisticsArr.PushBack(player.gameStatistics[i], d.GetAllocator());
		}
		attrObj.AddMember("statistics", statisticsArr, d.GetAllocator());

		character.AddMember("attributes", attrObj, d.GetAllocator());
	}

	rapidjson::Value inventory(rapidjson::kArrayType);
	for ( const auto& item : myStats.inventory )
	{
		rapidjson::Value itemArray(rapidjson::kArrayType);
		itemArray.PushBack((int)item.type, d.GetAllocator());
		itemArray.PushBack((int)item.status, d.GetAllocator());
		itemArray.PushBack((int)item.beatitude, d.GetAllocator());
		itemArray.PushBack((int)item.count, d.GetAllocator());
		itemArray.PushBack((int)item.appearance, d.GetAllocator());
		itemArray.PushBack((int)item.identified, d.GetAllocator());
		itemArray.PushBack((int)0 /* blank uid */, d.GetAllocator());
		itemArray.PushBack((int)item.x, d.GetAllocator());
		itemArray.PushBack((int)item.y, d.GetAllocator());

		inventory.PushBack(itemArray, d.GetAllocator());
	}

	{
		// equip slots
		for ( const auto& equipment : player.stats.player_equipment )
		{
			rapidjson::Value itemArray(rapidjson::kArrayType);
			if ( equipment.second != UINT32_MAX && equipment.second < player.stats.inventory.size() )
			{
				auto& item = player.stats.inventory[equipment.second];
				itemArray.PushBack((int)item.type, d.GetAllocator());
				itemArray.PushBack((int)item.status, d.GetAllocator());
				itemArray.PushBack((int)item.beatitude, d.GetAllocator());
				itemArray.PushBack((int)item.count, d.GetAllocator());
				itemArray.PushBack((int)item.appearance, d.GetAllocator());
				itemArray.PushBack((int)item.identified, d.GetAllocator());
				itemArray.PushBack((int)0 /* blank uid */, d.GetAllocator());
				itemArray.PushBack((int)item.x, d.GetAllocator());
				itemArray.PushBack((int)item.y, d.GetAllocator());
			}
			rapidjson::Value key(equipment.first.c_str(), d.GetAllocator());
			character.AddMember(key, itemArray, d.GetAllocator());
		}
	}

	character.AddMember("inventory", inventory, d.GetAllocator());

	d.AddMember("score", character, d.GetAllocator());

	rapidjson::StringBuffer os;
	rapidjson::Writer<rapidjson::StringBuffer> writer(os);
	d.Accept(writer);
	return os.GetString();
}

int loadGame(int player, const SaveGameInfo& info) {
	if (player < 0 || player >= MAXPLAYERS) {
		printlog("loadGame() failed: invalid player index");
		return 1;
	}

	if (info.magic_cookie != "BARONYJSONSAVE") {
		printlog("loadGame() failed: magic cookie is incorrect");
		return 1;
	}

	//if (info.game_version != getSavegameVersion(VERSION)) {
	//	printlog("loadGame() failed: game version mismatch");
	//	return 1;
	//}

	if ( (info.players_connected.size() <= player) || !info.players_connected[player]) {
		if ( !info.hiscore_dummy_loading )
		{
			printlog("loadGame() failed: given player is not connected");
		}
		return 1;
	}

	int statsPlayer = player;
	if ( info.hiscore_dummy_loading )
	{
		statsPlayer = MAXPLAYERS - 1; // read into non 0 player slot to not affect leaderboard
	}

	stats[statsPlayer]->clearStats();

	// load game info
	uniqueGameKey = info.gamekey;
	uniqueLobbyKey = info.lobbykey;
	mapseed = info.mapseed;
	completionTime = info.gametimer;
	clientnum = info.player_num;
	if ( !info.hiscore_dummy_loading )
	{
		switch (info.multiplayer_type) {
		default:
		case SINGLE: multiplayer = SINGLE; splitscreen = false; directConnect = false; break;
		case SERVER: multiplayer = SERVER; splitscreen = false; directConnect = false; break;
		case CLIENT: multiplayer = CLIENT; splitscreen = false; directConnect = false; break;
		case DIRECTSERVER: multiplayer = SERVER; splitscreen = false; directConnect = true; break;
		case DIRECTCLIENT: multiplayer = CLIENT; splitscreen = false; directConnect = true; break;
		case SERVERCROSSPLAY: multiplayer = SERVER; splitscreen = false; directConnect = false; break; // TODO!
		case SPLITSCREEN: multiplayer = SINGLE; splitscreen = true; directConnect = false; break;
		}
	}
	currentlevel = info.dungeon_lvl;
	secretlevel = info.level_track != 0;

	if ( !info.hiscore_dummy_loading )
	{
		if ( clientnum == player )
		{
			gameModeManager.currentSession.saveServerFlags();
			if ( multiplayer == CLIENT )
			{
				lobbyWindowSvFlags = info.svflags;
			}
			else
			{
				svFlags = info.svflags;
			}
			printlog("[SESSION]: Using savegame server flags");
			gameModeManager.currentSession.seededRun.seed = info.customseed;
			gameModeManager.currentSession.seededRun.seedString = info.customseed_string;
			for ( auto& pair : info.additional_data )
			{
				if ( pair.first == "game_scenario" )
				{
					gameModeManager.currentSession.challengeRun.setup(pair.second);
					if ( gameModeManager.currentSession.challengeRun.isActive() )
					{
						if ( gameModeManager.currentSession.challengeRun.lid.find("oneshot") != std::string::npos )
						{
							gameModeManager.setMode(GameModeManager_t::GAME_MODE_CUSTOM_RUN_ONESHOT);
						}
						else if ( gameModeManager.currentSession.challengeRun.lid.find("unlimited") != std::string::npos )
						{
							gameModeManager.setMode(GameModeManager_t::GAME_MODE_CUSTOM_RUN);
						}
						else if ( gameModeManager.currentSession.challengeRun.lid.find("challenge") != std::string::npos )
						{
							gameModeManager.setMode(GameModeManager_t::GAME_MODE_CUSTOM_RUN);
						}
					}
				}
			}
		}
	}

	// load player data
	client_classes[statsPlayer] = info.players[player].char_class;
	stats[statsPlayer]->playerRace = info.players[player].race;
	for ( int c = 0; c < NUMMONSTERS; ++c )
	{
		kills[c] = 0;
	}
	for (int c = 0; c < NUMMONSTERS && c < info.players[player].kills.size(); ++c) {
		kills[c] = info.players[player].kills[c];
	}
	conductPenniless = info.players[player].conductPenniless;
	conductFoodless = info.players[player].conductFoodless;
	conductVegetarian = info.players[player].conductVegetarian;
	conductIlliterate = info.players[player].conductIlliterate;
	for (int c = 0; c < NUM_CONDUCT_CHALLENGES; ++c) {
		conductGameChallenges[c] = info.players[player].additionalConducts[c];
	}
	for (int c = 0; c < NUM_GAMEPLAY_STATISTICS; ++c) {
		gameStatistics[c] = info.players[player].gameStatistics[c];
	}

	// read spells
	list_FreeAll(&players[statsPlayer]->magic.spellList);
	Uint32 spellIndex = 0;
	for (auto& s : info.players[player].spells) {
		spell_t* spell = copySpell(getSpellFromID(s));
		node_t* node = list_AddNodeLast(&players[statsPlayer]->magic.spellList);
		node->element = spell;
		node->deconstructor = &spellDeconstructor;
		node->size = sizeof(spell);

		if ( info.players[player].selected_spell == spellIndex )
		{
			players[statsPlayer]->magic.equipSpell(spell);
		}
		for ( int i = 0; i < NUM_HOTBAR_ALTERNATES; ++i )
		{
			if ( info.players[player].selected_spell_alternate[i] == spellIndex )
			{
				players[statsPlayer]->magic.selected_spell_alternate[i] = spell;
			}
		}

		++spellIndex;
	}
	players[statsPlayer]->magic.selected_spell_last_appearance = info.players[player].selected_spell_last_appearance;

	// read alchemy recipes
	clientLearnedAlchemyRecipes[statsPlayer].clear();
	for (auto& r : info.players[player].known_recipes) {
		clientLearnedAlchemyRecipes[statsPlayer].push_back(r);
	}

	// read scroll labels
	clientLearnedScrollLabels[statsPlayer].clear();
	for (auto& s : info.players[player].known_scrolls) {
		clientLearnedScrollLabels[statsPlayer].insert(s);
	}

	// player stats
	auto& p = info.players[player].stats;
	stringCopy(stats[statsPlayer]->name, p.name.c_str(), sizeof(Stat::name), p.name.size());
	stats[statsPlayer]->sex = static_cast<sex_t>(p.sex);
	stats[statsPlayer]->stat_appearance = p.statscore_appearance;
	stats[statsPlayer]->HP = p.HP;
	stats[statsPlayer]->MAXHP = p.maxHP;
	stats[statsPlayer]->MP = p.MP;
	stats[statsPlayer]->MAXMP = p.maxMP;
	stats[statsPlayer]->STR = p.STR;
	stats[statsPlayer]->DEX = p.DEX;
	stats[statsPlayer]->CON = p.CON;
	stats[statsPlayer]->INT = p.INT;
	stats[statsPlayer]->PER = p.PER;
	stats[statsPlayer]->CHR = p.CHR;
	stats[statsPlayer]->EXP = p.EXP;
	stats[statsPlayer]->LVL = p.LVL;
	stats[statsPlayer]->GOLD = p.GOLD;
	stats[statsPlayer]->HUNGER = p.HUNGER;
	for (int c = 0; c < NUMPROFICIENCIES && c < p.PROFICIENCIES.size(); ++c) {
		stats[statsPlayer]->setProficiency(c, p.PROFICIENCIES[c]);
	}

	for (int c = 0; c < NUMEFFECTS; ++c) {
		if ( c < p.EFFECTS.size() )
		{
			stats[statsPlayer]->EFFECTS[c] = p.EFFECTS[c];
			stats[statsPlayer]->EFFECTS_TIMERS[c] = p.EFFECTS_TIMERS[c];
		}
		else
		{
			stats[statsPlayer]->EFFECTS[c] = 0;
			stats[statsPlayer]->EFFECTS_TIMERS[c] = 0;
		}
	}
	constexpr int NUMMISCFLAGS = sizeof(Stat::MISC_FLAGS) / sizeof(Stat::MISC_FLAGS[0]);
	for (int c = 0; c < NUMMISCFLAGS && c < p.MISC_FLAGS.size(); ++c) {
		stats[statsPlayer]->MISC_FLAGS[c] = p.MISC_FLAGS[c];
	}
	//stats[statsPlayer]->attributes = p.attributes; // skip attributes for now
	for ( auto& loot : p.player_lootbags )
	{
		auto& player_lootbag = stats[statsPlayer]->player_lootbags[loot.first];
		player_lootbag.spawn_x = loot.second.spawn_x;
		player_lootbag.spawn_y = loot.second.spawn_y;
		player_lootbag.spawnedOnGround = loot.second.spawnedOnGround;
		player_lootbag.looted = loot.second.looted;

		for ( auto& _item : loot.second.items )
		{
			player_lootbag.items.push_back(Item());
			auto& item = player_lootbag.items.back();

			item.type = static_cast<ItemType>(_item.type);
			item.status = static_cast<Status>(_item.status);
			item.beatitude = _item.beatitude;
			item.count = _item.count;
			item.appearance = _item.appearance;
			item.identified = _item.identified;
		}
	}

	// inventory
	for (auto& item : p.inventory) {
		ItemType type = static_cast<ItemType>(item.type);
		Status status = static_cast<Status>(item.status);
		Sint16 beatitude = item.beatitude;
		Sint16 count = item.count;
		Uint32 appearance = item.appearance;
		bool identified = item.identified;
		Item* i = newItem(type, status, beatitude, count,
			appearance, identified, &stats[statsPlayer]->inventory);
		i->x = item.x;
		i->y = item.y;
	}

	// equipment
	const std::unordered_map<std::string, Item*&> slots = {
		{"helmet", stats[statsPlayer]->helmet},
		{"breastplate", stats[statsPlayer]->breastplate},
		{"gloves", stats[statsPlayer]->gloves},
		{"shoes", stats[statsPlayer]->shoes},
		{"shield", stats[statsPlayer]->shield},
		{"weapon", stats[statsPlayer]->weapon},
		{"cloak", stats[statsPlayer]->cloak},
		{"amulet", stats[statsPlayer]->amulet},
		{"ring", stats[statsPlayer]->ring},
		{"mask", stats[statsPlayer]->mask},
	};
	if (players[statsPlayer]->isLocalPlayer()) {
		// if this is a local player, we have their inventory, and can
		// restore equipment using item indexes in the player_equipment table
		for (auto& item : p.player_equipment) {
			auto find = slots.find(item.first);
			if (find != slots.end()) {
				auto& slot = find->second;
				auto node = list_Node(&stats[statsPlayer]->inventory, item.second);
				if (node) {
					slot = (Item*)node->element;
				} else {
					slot = nullptr;
				}
			}
		}
	} else {
		// if this is not a local player, we don't have the inventory.
		// we must restore whole items and assign them directly to each slot
		for (auto& item : p.npc_equipment) {
			auto find = slots.find(item.first);
			if (find != slots.end()) {
				auto& slot = find->second;
				ItemType type = (ItemType)item.second.type;
				Status status = (Status)item.second.status;
				Sint16 beatitude = item.second.beatitude;
				Sint16 count = item.second.count;
				Uint32 appearance = item.second.appearance;
				bool identified = item.second.identified;
				Item* i = newItem(type, status, beatitude, count,
					appearance, identified, nullptr);
				i->x = item.second.x;
				i->y = item.second.y;
				slot = i;
			}
		}
	}

	// assign hotbar items
	auto& hotbar = players[statsPlayer]->hotbar.slots();
	auto& hotbar_alternate = players[statsPlayer]->hotbar.slotsAlternate();
	for (int c = 0; c < NUM_HOTBAR_SLOTS; ++c) {
		node_t* node = list_Node(&stats[statsPlayer]->inventory,
			info.players[player].hotbar[c]);
		if (node) {
			Item* item = (Item*)node->element;
			hotbar[c].item = item->uid;
			hotbar[c].storeLastItem(item);
		} else {
			hotbar[c].item = 0;
			hotbar[c].resetLastItem();
		}

		for ( int d = 0; d < NUM_HOTBAR_ALTERNATES; ++d )
		{
			node_t* node = list_Node(&stats[statsPlayer]->inventory,
				info.players[player].hotbar_alternate[d][c]);
			if ( node ) {
				Item* item = (Item*)node->element;
				hotbar_alternate[d][c].item = item->uid;
				hotbar_alternate[d][c].storeLastItem(item);
			}
			else
			{
				hotbar_alternate[d][c].item = 0;
				hotbar_alternate[d][c].resetLastItem();
			}
		}
	}

	// reset certain variables
	list_FreeAll(&stats[statsPlayer]->FOLLOWERS);
	stats[statsPlayer]->monster_sound = nullptr;
	stats[statsPlayer]->monster_idlevar = 0;
	stats[statsPlayer]->leader_uid = 0;

	// shuffle enchanted feather list
    {
	    enchantedFeatherScrollsShuffled.clear();
	    enchantedFeatherScrollsShuffled.reserve(enchantedFeatherScrollsFixedList.size());
	    auto shuffle = enchantedFeatherScrollsFixedList;
		BaronyRNG feather_rng;
		feather_rng.seedBytes(&uniqueGameKey, sizeof(uniqueGameKey));
	    while (!shuffle.empty()) {
	        int index = feather_rng.getU8() % shuffle.size();
	        enchantedFeatherScrollsShuffled.push_back(shuffle[index]);
	        shuffle.erase(shuffle.begin() + index);
	    }
	}

	// generate mimics
	{
		mimic_generator.init();
	}

	// shopkeeper hostility
	{
		auto& h = ShopkeeperPlayerHostility.playerHostility[statsPlayer];
		h.clear();
		for ( auto& hostility : info.players[player].shopkeeperHostility )
		{
			h[(Uint32)hostility.first] = ShopkeeperPlayerHostility_t::PlayerRaceHostility_t();
			h[(Uint32)hostility.first].wantedLevel = (ShopkeeperPlayerHostility_t::WantedLevel)hostility.second.wantedLevel;
			if ( info.game_version < 412 )
			{
				if ( h[(Uint32)hostility.first].wantedLevel > ShopkeeperPlayerHostility_t::NO_WANTED_LEVEL )
				{
					// we increased the wanted levels in 412
					h[(Uint32)hostility.first].wantedLevel = 
						(ShopkeeperPlayerHostility_t::WantedLevel)(h[(Uint32)hostility.first].wantedLevel + 1);
				}
			}
			h[(Uint32)hostility.first].playerRace = (Monster)hostility.second.playerRace;
			if ( info.game_version < 412 )
			{
				h[(Uint32)hostility.first].type = h[(Uint32)hostility.first].playerRace;
			}
			else
			{
				h[(Uint32)hostility.first].sex = (sex_t)hostility.second.sex;
				h[(Uint32)hostility.first].equipment = (Uint8)hostility.second.equipment;
				h[(Uint32)hostility.first].type = (Uint32)hostility.second.type;
			}
			h[(Uint32)hostility.first].player = hostility.second.player;
			h[(Uint32)hostility.first].numAggressions = hostility.second.numAggressions;
			h[(Uint32)hostility.first].numKills = hostility.second.numKills;
			h[(Uint32)hostility.first].numAccessories = hostility.second.numAccessories;
		}
	}

	// compendium progress
	{
		auto& compendiumProgress = players[statsPlayer]->compendiumProgress;
		for ( auto& compendium_item_events : info.players[player].compendium_item_events )
		{
			for ( auto itr = compendium_item_events.second.begin(); itr != compendium_item_events.second.end(); )
			{
				int first = *itr;
				++itr;
				if ( itr != compendium_item_events.second.end() )
				{
					Sint32 second = *itr;
					compendiumProgress.itemEvents[compendium_item_events.first][first] = second;
				}
				++itr;
			}
		}
	}

	// player rng stuff
	{
		auto& mechanics = players[statsPlayer]->mechanics;
		mechanics.itemDegradeRng.clear();
		for ( auto& pair : info.players[player].itemDegradeRNG )
		{
			mechanics.itemDegradeRng[pair.first] = pair.second;
		}
		mechanics.sustainedSpellMPUsed = 0;
		mechanics.sustainedSpellMPUsed = info.players[player].sustainedSpellMPUsed;
	}

	Player::Minimap_t::mapDetails = info.map_messages;

	if ( !info.hiscore_dummy_loading )
	{
		if ( !info.hash ) {
			printlog("loadGame() warning: hash check failed");
			gameStatistics[STATISTICS_DISABLE_UPLOAD] = 1;
		}
	}

	return 0;
}

list_t* loadGameFollowers(const SaveGameInfo& info) {
	if (info.magic_cookie != "BARONYJSONSAVE") {
		printlog("loadGameFollowers() failed: magic cookie is incorrect");
		return nullptr;
	}

	//if (info.game_version != getSavegameVersion(VERSION)) {
	//	printlog("loadGameFollowers() failed: game version mismatch");
	//	return nullptr;
	//}

	if (info.players_connected.size() != info.players.size()) {
		printlog("loadGameFollowers() failed: player data is malformed");
		return nullptr;
	}

	// create followers list
	list_t* followers = (list_t*) malloc(sizeof(list_t));
	followers->first = NULL;
	followers->last = NULL;

	// read the follower data
	for (auto& player : info.players) {
		list_t* followerList = (list_t*) malloc(sizeof(list_t));
		followerList->first = nullptr;
		followerList->last = nullptr;
		node_t* node = list_AddNodeLast(followers);
		node->element = followerList;
		node->deconstructor = &listDeconstructor;
		node->size = sizeof(list_t);

		// number of followers for this player
		for (auto& follower : player.followers) {
			// Stat init to 0 as monster type not needed,
			// values will be overwritten by the saved follower data
			Stat* stats = new Stat(0);

			node_t* node = list_AddNodeLast(followerList);
			node->element = stats;
			node->deconstructor = &statDeconstructor;
			node->size = sizeof(*stats);

			// read follower stats
			stringCopy(stats->name, follower.name.c_str(),
				sizeof(Stat::name), follower.name.size());
			stats->type = (Monster)follower.type;
			stats->sex = (sex_t)follower.sex;
			stats->stat_appearance = follower.statscore_appearance;
			stats->HP = follower.HP;
			stats->MAXHP = follower.maxHP;
			stats->MP = follower.MP;
			stats->MAXMP = follower.maxMP;
			stats->STR = follower.STR;
			stats->DEX = follower.DEX;
			stats->CON = follower.CON;
			stats->INT = follower.INT;
			stats->PER = follower.PER;
			stats->CHR = follower.CHR;
			stats->EXP = follower.EXP;
			stats->LVL = follower.LVL;
			stats->GOLD = follower.GOLD;
			stats->HUNGER = follower.HUNGER;
			for (int c = 0; c < NUMPROFICIENCIES && c < follower.PROFICIENCIES.size(); ++c) {
				stats->setProficiency(c, follower.PROFICIENCIES[c]);
			}
			for (int c = 0; c < NUMEFFECTS; ++c) {
				if ( c < follower.EFFECTS.size() )
				{
					stats->EFFECTS[c] = follower.EFFECTS[c];
					stats->EFFECTS_TIMERS[c] = follower.EFFECTS_TIMERS[c];
				}
				else
				{
					stats->EFFECTS[c] = 0;
					stats->EFFECTS_TIMERS[c] = 0;
				}
			}
			constexpr int NUMMISCFLAGS = sizeof(Stat::MISC_FLAGS) / sizeof(Stat::MISC_FLAGS[0]);
			for (int c = 0; c < NUMMISCFLAGS && c < follower.MISC_FLAGS.size(); ++c) {
				stats->MISC_FLAGS[c] = follower.MISC_FLAGS[c];
			}

			// read follower attributes
			for (auto& attr : follower.attributes) {
				char key[32];
				char value[32];
				stringCopy(key, attr.first.c_str(), sizeof(key), attr.first.size());
				stringCopy(value, attr.second.c_str(), sizeof(value), attr.second.size());
				stats->attributes.emplace(std::make_pair(key, value));
			}

			// read follower inventory
			for (auto& item : follower.inventory) {
				ItemType type = (ItemType)item.type;
				Status status = (Status)item.status;
				Sint16 beatitude = item.beatitude;
				Sint16 count = item.count;
				Uint32 appearance = item.appearance;
				bool identified = item.identified;
				Item* i = newItem(type, status, beatitude, count,
					appearance, identified, &stats->inventory);
				i->x = item.x;
				i->y = item.y;
			}

			// equipment
			const std::unordered_map<std::string, Item*&> slots = {
				{"helmet", stats->helmet},
				{"breastplate", stats->breastplate},
				{"gloves", stats->gloves},
				{"shoes", stats->shoes},
				{"shield", stats->shield},
				{"weapon", stats->weapon},
				{"cloak", stats->cloak},
				{"amulet", stats->amulet},
				{"ring", stats->ring},
				{"mask", stats->mask},
			};
			for (auto& item : follower.npc_equipment) {
				auto find = slots.find(item.first);
				if (find != slots.end()) {
					auto& slot = find->second;
					ItemType type = (ItemType)item.second.type;
					Status status = (Status)item.second.status;
					Sint16 beatitude = item.second.beatitude;
					Sint16 count = item.second.count;
					Uint32 appearance = item.second.appearance;
					bool identified = item.second.identified;
					Item* i = newItem(type, status, beatitude, count,
						appearance, identified, nullptr);
					i->x = item.second.x;
					i->y = item.second.y;
					slot = i;
				}
			}
		}
	}

	return followers;
} 

int SaveGameInfo::Player::isCharacterValidFromDLC()
{
	switch ( this->char_class )
	{
	case CLASS_CONJURER:
	case CLASS_ACCURSED:
	case CLASS_MESMER:
	case CLASS_BREWER:
		if ( !enabledDLCPack1 )
		{
			return INVALID_REQUIREDLC1;
		}
		break;
	case CLASS_MACHINIST:
	case CLASS_PUNISHER:
	case CLASS_SHAMAN:
	case CLASS_HUNTER:
		if ( !enabledDLCPack2 )
		{
			return INVALID_REQUIREDLC2;
		}
		break;
	default:
		break;
	}

	switch ( this->race )
	{
	case RACE_SKELETON:
	case RACE_VAMPIRE:
	case RACE_SUCCUBUS:
	case RACE_GOATMAN:
		if ( !enabledDLCPack1 )
		{
			return INVALID_REQUIREDLC1;
		}
		break;
	case RACE_AUTOMATON:
	case RACE_INCUBUS:
	case RACE_GOBLIN:
	case RACE_INSECTOID:
		if ( !enabledDLCPack2 )
		{
			return INVALID_REQUIREDLC2;
		}
		break;
	default:
		break;
	}

	if ( this->race == RACE_HUMAN )
	{
		return VALID_OK_CHARACTER;
	}
	else if ( this->race > RACE_HUMAN && this->stats.statscore_appearance == 1 )
	{
		return VALID_OK_CHARACTER; // aesthetic only option.
	}
	if ( this->char_class <= CLASS_MONK )
	{
		return VALID_OK_CHARACTER;
	}

	switch ( this->char_class )
	{
	case CLASS_CONJURER:
		if ( this->race == RACE_SKELETON )
		{
			return VALID_OK_CHARACTER;
		}
		return isAchievementUnlockedForClassUnlock(RACE_SKELETON) ? VALID_OK_CHARACTER : INVALID_REQUIRE_ACHIEVEMENT;
		break;
	case CLASS_ACCURSED:
		if ( this->race == RACE_VAMPIRE )
		{
			return VALID_OK_CHARACTER;
		}
		return isAchievementUnlockedForClassUnlock(RACE_VAMPIRE) ? VALID_OK_CHARACTER : INVALID_REQUIRE_ACHIEVEMENT;
		break;
	case CLASS_MESMER:
		if ( this->race == RACE_SUCCUBUS )
		{
			return VALID_OK_CHARACTER;
		}
		return isAchievementUnlockedForClassUnlock(RACE_SUCCUBUS) ? VALID_OK_CHARACTER : INVALID_REQUIRE_ACHIEVEMENT;
		break;
	case CLASS_BREWER:
		if ( this->race == RACE_GOATMAN )
		{
			return VALID_OK_CHARACTER;
		}
		return isAchievementUnlockedForClassUnlock(RACE_GOATMAN) ? VALID_OK_CHARACTER : INVALID_REQUIRE_ACHIEVEMENT;
		break;
	case CLASS_MACHINIST:
		if ( this->race == RACE_AUTOMATON )
		{
			return VALID_OK_CHARACTER;
		}
		return isAchievementUnlockedForClassUnlock(RACE_AUTOMATON) ? VALID_OK_CHARACTER : INVALID_REQUIRE_ACHIEVEMENT;
		break;
	case CLASS_PUNISHER:
		if ( this->race == RACE_INCUBUS )
		{
			return VALID_OK_CHARACTER;
		}
		return isAchievementUnlockedForClassUnlock(RACE_INCUBUS) ? VALID_OK_CHARACTER : INVALID_REQUIRE_ACHIEVEMENT;
		break;
	case CLASS_SHAMAN:
		if ( this->race == RACE_GOBLIN )
		{
			return VALID_OK_CHARACTER;
		}
		return isAchievementUnlockedForClassUnlock(RACE_GOBLIN) ? VALID_OK_CHARACTER : INVALID_REQUIRE_ACHIEVEMENT;
		break;
	case CLASS_HUNTER:
		if ( this->race == RACE_INSECTOID )
		{
			return VALID_OK_CHARACTER;
		}
		return isAchievementUnlockedForClassUnlock(RACE_INSECTOID) ? VALID_OK_CHARACTER : INVALID_REQUIRE_ACHIEVEMENT;
		break;
	default:
		break;
	}

	return INVALID_CHARACTER;
}