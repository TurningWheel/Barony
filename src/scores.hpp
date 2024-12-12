/*-------------------------------------------------------------------------------

	BARONY
	File: scores.hpp
	Desc: header file for scores.cpp

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#pragma once
#include "monster.hpp"
#include "json.hpp"
#include "player.hpp"

#define SCORESFILE "scores.dat"
#define SCORESFILE_MULTIPLAYER "scores_multiplayer.dat"

// game score structure
#define MAXTOPSCORES 100
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
static const int CONDUCT_MODDED_NO_ACHIEVEMENTS = 12; // 1 = mods have disabled achievements
static const int CONDUCT_ASSISTANCE_CLAIMED = 13; // how many assistance points claimed for the party

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
static const int STATISTICS_TOTAL_KILLS = 16;
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
	STEAM_STAT_OVERCLOCKED,
	STEAM_STAT_BACK_TO_BASICS,
	STEAM_STAT_EXTRA_CREDIT,
	STEAM_STAT_EXTRA_CREDIT_LVLS,
	STEAM_STAT_DIPLOMA,
	STEAM_STAT_DIPLOMA_LVLS,
	STEAM_STAT_TUTORIAL_ENTERED,
	STEAM_STAT_I_NEEDED_THAT,
	STEAM_STAT_DAPPER_1,
	STEAM_STAT_DAPPER_2,
	STEAM_STAT_DAPPER_3,
	STEAM_STAT_DAPPER,
	STEAM_STAT_DUNGEONSEED,
	STEAM_STAT_PITCH_PERFECT,
	STEAM_STAT_RUNG_OUT,
	STEAM_STAT_SMASH_MELEE
};

enum SteamGlobalStatIndexes : int
{
	STEAM_GSTAT_INVALID = -1,
	STEAM_GSTAT_GAMES_STARTED = 0,
	STEAM_GSTAT_GAMES_WON,
	STEAM_GSTAT_BOULDER_DEATHS,
	STEAM_GSTAT_HERX_SLAIN,
	STEAM_GSTAT_BAPHOMET_SLAIN,
	STEAM_GSTAT_TWINSFIRE_SLAIN,
	STEAM_GSTAT_DEATHS_HUMAN,
	STEAM_GSTAT_DEATHS_RAT,
	STEAM_GSTAT_DEATHS_GOBLIN,
	STEAM_GSTAT_DEATHS_SLIME,
	STEAM_GSTAT_DEATHS_TROLL,
	STEAM_GSTAT_DEATHS_SPIDER,
	STEAM_GSTAT_DEATHS_GHOUL,
	STEAM_GSTAT_DEATHS_SKELETON,
	STEAM_GSTAT_DEATHS_SCORPION,
	STEAM_GSTAT_DEATHS_IMP,
	STEAM_GSTAT_DEATHS_GNOME,
	STEAM_GSTAT_DEATHS_DEMON,
	STEAM_GSTAT_DEATHS_SUCCUBUS,
	STEAM_GSTAT_DEATHS_LICH,
	STEAM_GSTAT_DEATHS_MINOTAUR,
	STEAM_GSTAT_DEATHS_DEVIL,
	STEAM_GSTAT_DEATHS_SHOPKEEPER,
	STEAM_GSTAT_DEATHS_KOBOLD,
	STEAM_GSTAT_DEATHS_SCARAB,
	STEAM_GSTAT_DEATHS_CRYSTALGOLEM,
	STEAM_GSTAT_DEATHS_INCUBUS,
	STEAM_GSTAT_DEATHS_VAMPIRE,
	STEAM_GSTAT_DEATHS_SHADOW,
	STEAM_GSTAT_DEATHS_COCKATRICE,
	STEAM_GSTAT_DEATHS_INSECTOID,
	STEAM_GSTAT_DEATHS_GOATMAN,
	STEAM_GSTAT_DEATHS_AUTOMATON,
	STEAM_GSTAT_DEATHS_LICHICE,
	STEAM_GSTAT_DEATHS_LICHFIRE,
	STEAM_GSTAT_DEATHS_SENTRYBOT,
	STEAM_GSTAT_DEATHS_SPELLBOT,
	STEAM_GSTAT_DEATHS_GYROBOT,
	STEAM_GSTAT_DEATHS_DUMMYBOT,
	STEAM_GSTAT_TWINSICE_SLAIN,
	STEAM_GSTAT_SHOPKEEPERS_SLAIN,
	STEAM_GSTAT_MINOTAURS_SLAIN,
	STEAM_GSTAT_TUTORIAL_ENTERED,
	STEAM_GSTAT_TUTORIAL1_COMPLETED,
	STEAM_GSTAT_TUTORIAL2_COMPLETED,
	STEAM_GSTAT_TUTORIAL3_COMPLETED,
	STEAM_GSTAT_TUTORIAL4_COMPLETED,
	STEAM_GSTAT_TUTORIAL5_COMPLETED,
	STEAM_GSTAT_TUTORIAL6_COMPLETED,
	STEAM_GSTAT_TUTORIAL7_COMPLETED,
	STEAM_GSTAT_TUTORIAL8_COMPLETED,
	STEAM_GSTAT_TUTORIAL9_COMPLETED,
	STEAM_GSTAT_TUTORIAL10_COMPLETED,
	STEAM_GSTAT_TUTORIAL1_ATTEMPTS,
	STEAM_GSTAT_TUTORIAL2_ATTEMPTS,
	STEAM_GSTAT_TUTORIAL3_ATTEMPTS,
	STEAM_GSTAT_TUTORIAL4_ATTEMPTS,
	STEAM_GSTAT_TUTORIAL5_ATTEMPTS,
	STEAM_GSTAT_TUTORIAL6_ATTEMPTS,
	STEAM_GSTAT_TUTORIAL7_ATTEMPTS,
	STEAM_GSTAT_TUTORIAL8_ATTEMPTS,
	STEAM_GSTAT_TUTORIAL9_ATTEMPTS,
	STEAM_GSTAT_TUTORIAL10_ATTEMPTS,
	STEAM_GSTAT_DISABLE,
	STEAM_GSTAT_PROMO,
	STEAM_GSTAT_PROMO_INTERACT,
	STEAM_GSTAT_DEATHS_BAT,
	STEAM_GSTAT_DEATHS_BUGBEAR,
	STEAM_GSTAT_DEATHS_MIMIC,
	STEAM_GSTAT_MAX
};

const std::vector<std::string> SteamGlobalStatStr =
{
	"GAMES_STARTED",
	"GAMES_WON",
	"BOULDER_DEATHS",
	"HERX_SLAIN",
	"BAPHOMET_SLAIN",
	"TWINSFIRE_SLAIN",
	"DEATHS_HUMAN",
	"DEATHS_RAT",
	"DEATHS_GOBLIN",
	"DEATHS_SLIME",
	"DEATHS_TROLL",
	"DEATHS_SPIDER",
	"DEATHS_GHOUL",
	"DEATHS_SKELETON",
	"DEATHS_SCORPION",
	"DEATHS_IMP",
	"DEATHS_GNOME",
	"DEATHS_DEMON",
	"DEATHS_SUCCUBUS",
	"DEATHS_LICH",
	"DEATHS_MINOTAUR",
	"DEATHS_DEVIL",
	"DEATHS_SHOPKEEPER",
	"DEATHS_KOBOLD",
	"DEATHS_SCARAB",
	"DEATHS_CRYSTALGOLEM",
	"DEATHS_INCUBUS",
	"DEATHS_VAMPIRE",
	"DEATHS_SHADOW",
	"DEATHS_COCKATRICE",
	"DEATHS_INSECTOID",
	"DEATHS_GOATMAN",
	"DEATHS_AUTOMATON",
	"DEATHS_LICHICE",
	"DEATHS_LICHFIRE",
	"DEATHS_SENTRYBOT",
	"DEATHS_SPELLBOT",
	"DEATHS_GYROBOT",
	"DEATHS_DUMMYBOT",
	"TWINSICE_SLAIN",
	"SHOPKEEPERS_SLAIN",
	"MINOTAURS_SLAIN",
	"TUTORIAL_ENTERED",
	"TUTORIAL1_COMPLETED",
	"TUTORIAL2_COMPLETED",
	"TUTORIAL3_COMPLETED",
	"TUTORIAL4_COMPLETED",
	"TUTORIAL5_COMPLETED",
	"TUTORIAL6_COMPLETED",
	"TUTORIAL7_COMPLETED",
	"TUTORIAL8_COMPLETED",
	"TUTORIAL9_COMPLETED",
	"TUTORIAL10_COMPLETED",
	"TUTORIAL1_ATTEMPTS",
	"TUTORIAL2_ATTEMPTS",
	"TUTORIAL3_ATTEMPTS",
	"TUTORIAL4_ATTEMPTS",
	"TUTORIAL5_ATTEMPTS",
	"TUTORIAL6_ATTEMPTS",
	"TUTORIAL7_ATTEMPTS",
	"TUTORIAL8_ATTEMPTS",
	"TUTORIAL9_ATTEMPTS",
	"TUTORIAL10_ATTEMPTS",
	"DISABLE",
	"PROMO",
	"PROMO_INTERACT",
	"DEATHS_BAT",
	"DEATHS_BUGBEAR",
	"DEATHS_MIMIC",
};

SteamGlobalStatIndexes getIndexForDeathType(int type);

static const std::pair<std::string, int> steamStatAchStringsAndMaxVals[] =
{
	std::make_pair("BARONY_ACH_NONE", 999999),				// STEAM_STAT_BOULDER_DEATHS,
	std::make_pair("BARONY_ACH_RHINESTONE_COWBOY", 50),		// STEAM_STAT_RHINESTONE_COWBOY,
	std::make_pair("BARONY_ACH_TOUGH_AS_NAILS", 50),		// STEAM_STAT_TOUGH_AS_NAILS,
	std::make_pair("BARONY_ACH_UNSTOPPABLE_FORCE",50),		// STEAM_STAT_UNSTOPPABLE_FORCE,
	std::make_pair("BARONY_ACH_NONE", 999999),				// STEAM_STAT_GAMES_STARTED,
	std::make_pair("BARONY_ACH_NONE", 999999),				// STEAM_STAT_GAMES_WON,
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
	std::make_pair("BARONY_ACH_MANY_PEDI_PALP", 50),		// STEAM_STAT_MANY_PEDI_PALP
	std::make_pair("BARONY_ACH_5000_SECOND_RULE", 50),      // STEAM_STAT_5000_SECOND_RULE
	std::make_pair("BARONY_ACH_SOCIAL_BUTTERFLY", 50),      // STEAM_STAT_SOCIAL_BUTTERFLY
	std::make_pair("BARONY_ACH_ROLL_THE_BONES", 50),        // STEAM_STAT_ROLL_THE_BONES
	std::make_pair("BARONY_ACH_COWBOY_FROM_HELL", 50),      // STEAM_STAT_COWBOY_FROM_HELL
	std::make_pair("BARONY_ACH_SELF_FLAGELLATION", 30),     // STEAM_STAT_SELF_FLAGELLATION
	std::make_pair("BARONY_ACH_CHOPPING_BLOCK", 50),		// STEAM_STAT_CHOPPING_BLOCK
	std::make_pair("BARONY_ACH_IF_YOU_LOVE_SOMETHING", 100),// STEAM_STAT_IF_YOU_LOVE_SOMETHING
	std::make_pair("BARONY_ACH_RAGE_AGAINST", 20),          // STEAM_STAT_RAGE_AGAINST
	std::make_pair("BARONY_ACH_GUERILLA_RADIO", 20),        // STEAM_STAT_GUERILLA_RADIO
	std::make_pair("BARONY_ACH_FASCIST", 50),				// STEAM_STAT_FASCIST,
	std::make_pair("BARONY_ACH_ITS_A_LIVING", 50),			// STEAM_STAT_ITS_A_LIVING,
	std::make_pair("BARONY_ACH_OVERCLOCKED", 600),			// STEAM_STAT_OVERCLOCKED
	std::make_pair("BARONY_ACH_NONE", 1),					// STEAM_STAT_BACK_TO_BASICS,
	std::make_pair("BARONY_ACH_EXTRA_CREDIT", 10),			// STEAM_STAT_EXTRA_CREDIT
	std::make_pair("BARONY_ACH_NONE", 1023),				// STEAM_STAT_EXTRA_CREDIT_LVLS,
	std::make_pair("BARONY_ACH_DIPLOMA", 10),				// STEAM_STAT_DIPLOMA
	std::make_pair("BARONY_ACH_NONE", 1023),				// STEAM_STAT_DIPLOMA_LVLS,
	std::make_pair("BARONY_ACH_NONE", 1),					// STEAM_STAT_TUTORIAL_ENTERED,
	std::make_pair("BARONY_ACH_I_NEEDED_THAT", 10),			// STEAM_STAT_I_NEEDED_THAT
	std::make_pair("BARONY_ACH_NONE", 0xFFFFFFFF),			// STEAM_STAT_DAPPER_1
	std::make_pair("BARONY_ACH_NONE", 0xFFFFFFFF),			// STEAM_STAT_DAPPER_2
	std::make_pair("BARONY_ACH_NONE", 0xFFFFFFFF),			// STEAM_STAT_DAPPER_3
	std::make_pair("BARONY_ACH_DAPPER", 30),				// STEAM_STAT_DAPPER
	std::make_pair("BARONY_ACH_DUNGEONSEED", 12),			// STEAM_STAT_DUNGEONSEED
	std::make_pair("BARONY_ACH_BAT1000", 81),				// STEAM_STAT_PITCH_PERFECT
	std::make_pair("BARONY_ACH_RUNG_OUT", 20),				// STEAM_STAT_RUNG_OUT
	std::make_pair("BARONY_ACH_SMASH_MELEE", 500)			// STEAM_STAT_SMASH_MELEE
};

typedef struct score_t
{
	Sint32 kills[NUMMONSTERS];
	Stat* stats;
	Sint32 classnum;
	Sint32 dungeonlevel;
	int victory;
	int totalscore = -1;

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
extern Uint32 loadinglobbykey;
extern Sint32 conductGameChallenges[NUM_CONDUCT_CHALLENGES];
extern Sint32 gameStatistics[NUM_GAMEPLAY_STATISTICS];
extern std::vector<std::pair<Uint32, Uint32>> achievementRhythmOfTheKnightVec[MAXPLAYERS];
extern bool achievementStatusRhythmOfTheKnight[MAXPLAYERS];
extern bool achievementRhythmOfTheKnight[MAXPLAYERS];
extern std::map<Uint32, Uint32> achievementThankTheTankPair[MAXPLAYERS];
extern bool achievementStatusBaitAndSwitch[MAXPLAYERS];
extern Uint32 achievementBaitAndSwitchTimer[MAXPLAYERS];
extern std::unordered_set<int> clientLearnedAlchemyIngredients[MAXPLAYERS];
extern std::vector<std::pair<int, std::pair<int, int>>> clientLearnedAlchemyRecipes[MAXPLAYERS];
extern std::unordered_set<int> clientLearnedScrollLabels[MAXPLAYERS];
extern bool achievementStatusThankTheTank[MAXPLAYERS];
extern std::vector<Uint32> achievementStrobeVec[MAXPLAYERS];
extern bool achievementStatusStrobe[MAXPLAYERS];
extern bool playerFailedRangedOnlyConduct[MAXPLAYERS];
extern bool achievementBrawlerMode;
extern bool achievementPenniless;
extern bool achievementRangedMode[MAXPLAYERS];

score_t* scoreConstructor(int player);
void scoreDeconstructor(void* data);
int saveScore(int player);
int totalScore(score_t* score);
void loadScore(int score);
void loadScore(score_t* score);
bool deleteScore(bool multiplayer, int index);
void saveAllScores(const std::string& scoresfilename);
void loadAllScores(const std::string& scoresfilename);

enum SaveFileType {
    MAIN,
    FOLLOWERS,
    SCREENSHOT,
	JSON,
    SIZE_OF_TYPE
};

extern int savegameCurrentFileIndex;

std::string setSaveGameFileName(bool singleplayer, SaveFileType type, int saveIndex = savegameCurrentFileIndex);
int saveGameOld(int saveIndex = savegameCurrentFileIndex);
int loadGameOld(int player, int saveIndex = savegameCurrentFileIndex);
list_t* loadGameFollowersOld(int saveIndex = savegameCurrentFileIndex);

int deleteSaveGame(int gametype, int saveIndex = savegameCurrentFileIndex);
bool saveGameExists(bool singleplayer, int saveIndex = savegameCurrentFileIndex);
bool anySaveFileExists(bool singleplayer);
bool anySaveFileExists();

struct SaveGameInfo {
	std::string magic_cookie = "BARONYJSONSAVE";
	int game_version = -1;
    std::string timestamp;
	Uint32 hash = 0;
	std::string gamename;
	Uint32 gamekey = 0;
	Uint32 lobbykey = 0;
	Uint32 mapseed = 0;
	Uint32 gametimer = 0;
	Uint32 svflags = 0;
	Uint32 customseed = 0;
	std::string customseed_string = "";
    int player_num = 0;
    int multiplayer_type = SINGLE;
    int dungeon_lvl = 0;
	int level_track = 0;
	int hiscore_loadstatus = 0;
	int hiscore_totalscore = 0;
	int hiscore_rank = -1;
	int hiscore_victory = 0;
	int hiscore_killed_by = 0;
	int hiscore_killed_monster = 0;
	int hiscore_killed_item = 0;
	bool hiscore_dummy_loading = false;
    std::vector<int> players_connected;

	int populateFromSession(const int playernum);
	int getTotalScore(const int playernum, const int victory);
	std::string serializeToOnlineHiscore(const int playernum, const int victory);

	struct Player {
		Uint32 char_class = 0;
		Uint32 race = 0;
		std::vector<int> kills;

		bool conductPenniless = false;
		bool conductFoodless = false;
		bool conductVegetarian = false;
		bool conductIlliterate = false;
		int additionalConducts[NUM_CONDUCT_CHALLENGES] = { 0 };
		int gameStatistics[NUM_GAMEPLAY_STATISTICS] = { 0 };

		Uint32 hotbar[NUM_HOTBAR_SLOTS];
		Uint32 hotbar_alternate[NUM_HOTBAR_ALTERNATES][NUM_HOTBAR_SLOTS];
		Uint32 selected_spell;
		Uint32 selected_spell_alternate[NUM_HOTBAR_ALTERNATES];
		int selected_spell_last_appearance;
		std::vector<Uint32> spells;

		typedef std::pair<int, std::pair<int, int>> recipe_t;
		std::vector<recipe_t> known_recipes;
		std::vector<int> known_scrolls;

		struct PlayerRaceHostility_t
		{
			int numAggressions = 0;
			int numKills = 0;
			int numAccessories = 0;
			int playerRace = NOTHING;
			int sex = sex_t::MALE;
			int equipment = 0;
			int type = 0;
			int wantedLevel = ShopkeeperPlayerHostility_t::NO_WANTED_LEVEL;
			int player = -1;

			PlayerRaceHostility_t() = default;
			PlayerRaceHostility_t(const PlayerRaceHostility_t&) = default;
			PlayerRaceHostility_t(PlayerRaceHostility_t&&) = default;
			PlayerRaceHostility_t(ShopkeeperPlayerHostility_t::PlayerRaceHostility_t& h)
			{
				wantedLevel = h.wantedLevel;
				playerRace = h.playerRace;
				sex = h.sex;
				equipment = h.equipment;
				type = h.type;
				player = h.player;
				numAggressions = h.numAggressions;
				numKills = h.numKills;
				numAccessories = h.numAccessories;
			}
			bool serialize(FileInterface* fp)
			{
				fp->property("wanted_level", wantedLevel);
				fp->property("player_race", playerRace);
				fp->property("equipment", equipment);
				fp->property("type", type);
				fp->property("sex", sex);
				fp->property("player", player);
				fp->property("num_aggressions", numAggressions);
				fp->property("num_kills", numKills);
				fp->property("num_accessories", numAccessories);
				return true;
			}
		};
		std::vector<std::pair<int, PlayerRaceHostility_t>> shopkeeperHostility;
		std::vector<std::pair<std::string, std::vector<int>>> compendium_item_events;
		std::vector<std::pair<int, int>> itemDegradeRNG;
		int sustainedSpellMPUsed = 0;

		struct stat_t {
			struct item_t {
				item_t() = default;
				item_t(const item_t&) = default;
				item_t(item_t&&) = default;
				item_t(Uint32 _type,
					Uint32 _status,
					Uint32 _appearance,
					int _beatitude,
					int _count,
					bool _identified,
					int _x,
					int _y)
				{
					type = _type;
					status = _status;
					appearance = _appearance;
					beatitude = _beatitude;
					count = _count;
					identified = _identified;
					x = _x;
					y = _y;
				}

				Uint32 type = 0;
				Uint32 status = 0;
				Uint32 appearance = 0;
				int beatitude = 0;
				int count = 1;
				bool identified = false;
				int x = 0;
				int y = 0;

				bool serialize(FileInterface* fp) {
					fp->property("type", type);
					fp->property("status", status);
					fp->property("appearance", appearance);
					fp->property("beatitude", beatitude);
					fp->property("count", count);
					fp->property("identified", identified);
					fp->property("x", x);
					fp->property("y", y);
					return true;
				}
				void computeHash(Uint32& hash, Uint32& shift);
			};

			struct lootbag_t
			{
				lootbag_t() = default;
				lootbag_t(const lootbag_t&) = default;
				lootbag_t(lootbag_t&&) = default;
				lootbag_t(
					int _spawn_x,
					int _spawn_y,
					bool _spawnedOnGround,
					bool _looted
					)
				{
					spawn_x = _spawn_x;
					spawn_y = _spawn_y;
					spawnedOnGround = _spawnedOnGround;
					looted = _looted;
				}

				int spawn_x = 0;
				int spawn_y = 0;
				bool spawnedOnGround = false;
				bool looted = false;
				std::vector<item_t> items;
				bool serialize(FileInterface* fp) {
					fp->property("spawn_x", spawn_x);
					fp->property("spawn_y", spawn_y);
					fp->property("looted", looted);
					fp->property("spawned", spawnedOnGround);
					fp->property("items", items);
					return true;
				}
			};

			std::string name;
			Uint32 type = Monster::HUMAN;
			Uint32 sex = 0;
			Uint32 statscore_appearance = 0;
			int HP = 0;
			int maxHP = 0;
			int MP = 0;
			int maxMP = 0;
			int STR = 0;
			int DEX = 0;
			int CON = 0;
			int INT = 0;
			int PER = 0;
			int CHR = 0;
			int EXP = 0;
			int LVL = 0;
			int GOLD = 0;
			int HUNGER = 0;
			std::vector<int> PROFICIENCIES;
			std::vector<int> EFFECTS;
			std::vector<int> EFFECTS_TIMERS;
			std::vector<int> MISC_FLAGS;
			std::vector<std::pair<std::string, std::string>> attributes;
			std::vector<std::pair<std::string, Uint32>> player_equipment;
			std::vector<std::pair<std::string, item_t>> npc_equipment;
			std::vector<item_t> inventory;
			std::vector<std::pair<Uint32, lootbag_t>> player_lootbags;

			bool serialize(FileInterface* fp) {
				fp->property("name", name);
				fp->property("type", type);
				fp->property("sex", sex);
				fp->property("appearance", statscore_appearance);
				fp->property("HP", HP);
				fp->property("maxHP", maxHP);
				fp->property("MP", MP);
				fp->property("maxMP", maxMP);
				fp->property("STR", STR);
				fp->property("DEX", DEX);
				fp->property("CON", CON);
				fp->property("INT", INT);
				fp->property("PER", PER);
				fp->property("CHR", CHR);
				fp->property("EXP", EXP);
				fp->property("LVL", LVL);
				fp->property("GOLD", GOLD);
				fp->property("HUNGER", HUNGER);
				fp->property("PROFICIENCIES", PROFICIENCIES);
				fp->property("EFFECTS", EFFECTS);
				fp->property("EFFECTS_TIMERS", EFFECTS_TIMERS);
				fp->property("MISC_FLAGS", MISC_FLAGS);
				fp->property("player_equipment", player_equipment);
				fp->property("npc_equipment", npc_equipment);
				fp->property("inventory", inventory);
				fp->property("attributes", attributes);
				fp->property("lootbags", player_lootbags);
				return true;
			}
		};

		stat_t stats;
		std::vector<stat_t> followers;
		
		bool serialize(FileInterface* fp) {
			fp->property("char_class", char_class);
			fp->property("race", race);
			fp->property("kills", kills);
			fp->property("race", race);
			fp->property("conduct_penniless", conductPenniless);
			fp->property("conduct_foodless", conductFoodless);
			fp->property("conduct_vegetarian", conductVegetarian);
			fp->property("conduct_illiterate", conductIlliterate);
			fp->property("additional_conducts", additionalConducts);
			if ( fp->isReading() )
			{
				for ( int i = 0; i < NUM_HOTBAR_SLOTS; ++i )
				{
					hotbar[i] = UINT32_MAX;
					for ( int j = 0; j < NUM_HOTBAR_ALTERNATES; ++j )
					{
						hotbar_alternate[j][i] = UINT32_MAX;
					}
				}
				selected_spell = UINT32_MAX;
				for ( int j = 0; j < NUM_HOTBAR_ALTERNATES; ++j )
				{
					selected_spell_alternate[j] = UINT32_MAX;
				}
			}
			fp->property("hotbar", hotbar);
			fp->property("hotbar_alternate", hotbar_alternate);
			fp->property("selected_spell", selected_spell);
			fp->property("selected_spell_alternate", selected_spell_alternate);
			fp->property("selected_spell_last_appearance", selected_spell_last_appearance);
			fp->property("spells", spells);
			fp->property("recipes", known_recipes);
			fp->property("scrolls", known_scrolls);
			fp->property("stats", stats);
			fp->property("followers", followers);
			fp->property("game_statistics", gameStatistics);
			fp->property("shopkeeper_hostility", shopkeeperHostility);
			fp->property("compendium_item_events", compendium_item_events);
			fp->property("item_degrade_rng", itemDegradeRNG);
			fp->property("sustained_mp_used", sustainedSpellMPUsed);
			return true;
		}

		int isCharacterValidFromDLC();
	};
	std::vector<Player> players;
	std::vector<std::pair<std::string, std::string>> map_messages; // map modifiers "sound of pickaxes striking rock" "walls are fortified" etc
	std::vector<std::pair<std::string, std::string>> additional_data;
	
	bool serialize(FileInterface* fp) {
		fp->property("magic_cookie", magic_cookie);
		fp->property("game_version", game_version);
		fp->property("timestamp", timestamp);
		fp->property("hash", hash);
		fp->property("game_name", gamename);
		fp->property("gamekey", gamekey);
		fp->property("lobbykey", lobbykey);
		fp->property("mapseed", mapseed);
		fp->property("gametimer", gametimer);
		fp->property("svflags", svflags);
		fp->property("player_num", player_num);
		fp->property("multiplayer_type", multiplayer_type);
		fp->property("dungeon_lvl", dungeon_lvl);
		fp->property("level_track", level_track);
		fp->property("customseed", customseed);
		fp->property("customseed_string", customseed_string);
		fp->property("players_connected", players_connected);
		fp->property("players", players);
		fp->property("additional_data", additional_data);
		fp->property("map_messages", map_messages);
		return true;
	}

	void computeHash(const int playernum, Uint32& hash);
};

int saveGame(int saveIndex = savegameCurrentFileIndex);
int loadGame(int player, const SaveGameInfo& info);
list_t* loadGameFollowers(const SaveGameInfo& info);

score_t* scoreConstructor(int player, SaveGameInfo& info);
SaveGameInfo getSaveGameInfo(bool singleplayer, int saveIndex = savegameCurrentFileIndex);
const char* getSaveGameName(const SaveGameInfo& info);
int getSaveGameType(const SaveGameInfo& info);
int getSaveGameClientnum(const SaveGameInfo& info);
Uint32 getSaveGameMapSeed(const SaveGameInfo& info);
int getSaveGameVersionNum(const SaveGameInfo& info);

int getSavegameVersion(const char* checkstr); // returns -1 on invalid version, otherwise converts to 3 digit int
void setDefaultPlayerConducts(); // init values for foodless, penniless etc.
void updatePlayerConductsInMainLoop(); // check and update conduct flags throughout game that don't require a specific action. (tracking gold, server flags etc...)
void updateGameplayStatisticsInMainLoop(); // check for achievement values for gameplay statistics.
void updateAchievementRhythmOfTheKnight(int player, Entity* target, bool playerIsHit);
void updateAchievementThankTheTank(int player, Entity* target, bool targetKilled);
void updateAchievementBaitAndSwitch(int player, bool isTeleporting);
static const int SAVE_GAMES_MAX = 100;

#ifndef EDITOR
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
		BARONY_ACH_TRASH_COMPACTOR,
		BARONY_ACH_EXTRA_CREDIT,
		BARONY_ACH_DIPLOMA,
		BARONY_ACH_BACK_TO_BASICS,
		BARONY_ACH_FAST_LEARNER,
		BARONY_ACH_MASTER,
		BARONY_ACH_DAPPER,
		BARONY_ACH_SPROUTS,
		BARONY_ACH_BY_THE_BOOK
	};
	enum AchievementEvent : int
	{
		ACH_EVENT_NONE,
		REAL_BOY_HUMAN_RECRUIT,
		REAL_BOY_SHOP,
		FORUM_TROLL_BREAK_WALL,
		FORUM_TROLL_RECRUIT_TROLL,
		FORUM_TROLL_FEAR,
		EXTRA_CREDIT_SECRET,
		DIPLOMA_LEVEL_COMPLETE,
		BACK_TO_BASICS_LEVEL_COMPLETE,
		FAST_LEARNER_TIME_UPDATE,
		DAPPER_EQUIPMENT_CHECK,
		BY_THE_BOOK_COMPENDIUM_PAGE,
		BY_THE_BOOK_BREW
	};
	void updatePlayerAchievement(int player, Achievement achievement, AchievementEvent achEvent);
	bool bIsAchievementAllowedDuringTutorial(std::string achievementStr)
	{
		if ( !achievementStr.compare("BARONY_ACH_TEACHABLE_MOMENT") )
		{
			return true;
		}
		if ( !achievementStr.compare("BARONY_ACH_MASTER") )
		{
			return true;
		}
		if ( !achievementStr.compare("BARONY_ACH_FAST_LEARNER") )
		{
			return true;
		}
		if ( !achievementStr.compare("BARONY_ACH_BACK_TO_BASICS") )
		{
			return true;
		}
		if ( !achievementStr.compare("BARONY_ACH_EXPELLED") )
		{
			return true;
		}
		if ( !achievementStr.compare("BARONY_ACH_EXTRA_CREDIT") )
		{
			return true;
		}
		if ( !achievementStr.compare("BARONY_ACH_DIPLOMA") )
		{
			return true;
		}
		return false;
	}
	bool bIsStatisticAllowedDuringTutorial(SteamStatIndexes statistic)
	{
		switch ( statistic )
		{
			case STEAM_STAT_EXTRA_CREDIT:
			case STEAM_STAT_EXTRA_CREDIT_LVLS:
			case STEAM_STAT_DIPLOMA:
			case STEAM_STAT_DIPLOMA_LVLS:
			case STEAM_STAT_TUTORIAL_ENTERED:
				return true;
				break;
			default:
				return false;
				break;
		}
	}
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
		bool totalKillsTickUpdate = false;
		Uint32 ticksByTheBookViewed = 0;
		static bool allPlayersDeadEvent;

		std::pair<int, int> realBoy;
		std::unordered_map<Uint32, int> caughtInAMoshTargets;
		std::vector<Uint32> strungOutTicks;
		std::unordered_set<Uint32> ironicPunishmentTargets;
		std::pair<real_t, real_t> flutterShyCoordinates;
		std::pair<int, Uint32> gastricBypassSpell;
		std::unordered_set<Uint32> rat5000secondRule;
		std::unordered_set<Uint32> phantomMaskFirstStrikes;
		std::unordered_set<Uint32> bountyTargets;
		bool updatedBountyTargets = false;
		bool wearingBountyHat = false;
		static std::set<ItemType> startingClassItems;
		
		PlayerAchievements()
		{
			realBoy = std::make_pair(0, 0);
			gastricBypassSpell = std::make_pair(0, 0);
			flutterShyCoordinates = std::make_pair(0.0, 0.0);
		};
		bool checkPathBetweenObjects(Entity* player, Entity* target, int achievement);
		bool checkTraditionKill(Entity* player, Entity* target);
		int getItemIndexForDapperAchievement(Item* item);
	} playerAchievements[MAXPLAYERS];

	void updateClientBounties(bool firstSend);
	void clearPlayerAchievementData();
	void checkMapScriptsOnVariableSet();
	void updateGlobalStat(int index, int player);
};
extern AchievementObserver achievementObserver;
#endif

#ifdef STEAMWORKS
bool steamLeaderboardSetScore(score_t* score);
bool steamLeaderboardReadScore(int tags[CSteamLeaderboards::k_numLeaderboardTags]);
#endif // STEAMWORKS
