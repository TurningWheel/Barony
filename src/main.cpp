/*-------------------------------------------------------------------------------

	BARONY
	File: main.cpp
	Desc: contains various miscellaneous functions

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "hash.hpp"
#include "entity.hpp"
#include "prng.hpp"

// main definitions
Sint32 xres = 960;
Sint32 yres = 600;
int mainloop = 1;
bool initialized = false;
Uint32 ticks = 0;
bool stop = false;
char datadir[PATH_MAX];
char outputdir[PATH_MAX];

// language stuff
char languageCode[32] = { 0 };
char** language = nullptr;

// input stuff
int reversemouse = 0;
real_t mousespeed = 32;
Uint32 impulses[NUMIMPULSES];
Uint32 joyimpulses[NUM_JOY_IMPULSES];
Uint32 lastkeypressed = 0;
Sint8 keystatus[512];
char* inputstr = nullptr;
int inputlen = 0;
Sint8 mousestatus[6];
Entity** clickmap = nullptr;
bool capture_mouse = true;
string lastname;
int lastCreatedCharacterClass = -1;
int lastCreatedCharacterAppearance = -1;
int lastCreatedCharacterSex = -1;
int lastCreatedCharacterRace = -1;

// net stuff
Uint32 clientplayer = 0;
int numplayers = 0;
int clientnum = 0;
int multiplayer = -1;
SteamStat_t g_SteamStats[NUM_STEAM_STATISTICS] =
{
	{ 1, STEAM_STAT_INT, "STAT_BOULDER_DEATHS" },
	{ 2, STEAM_STAT_INT, "STAT_WORTHLESS_GLASS" },
	{ 3, STEAM_STAT_INT, "STAT_TOUGH_AS_NAILS" },
	{ 4, STEAM_STAT_INT, "STAT_UNSTOPPABLE_FORCE" },
	{ 5, STEAM_STAT_INT, "STAT_GAMES_STARTED" },
	{ 6, STEAM_STAT_INT, "STAT_GAMES_WON" },
	{ 7, STEAM_STAT_INT, "STAT_BOMBARDIER" },
	{ 8, STEAM_STAT_INT, "STAT_IN_THE_MIX" },
	{ 9, STEAM_STAT_INT, "STAT_FREE_REFILLS" },
	{ 10, STEAM_STAT_INT, "STAT_TAKE_THIS_OUTSIDE" },
	{ 11, STEAM_STAT_INT, "STAT_ALTER_EGO" },
	{ 12, STEAM_STAT_INT, "STAT_BLOOD_SPORT" },
	{ 13, STEAM_STAT_INT, "STAT_BAD_BLOOD" },
	{ 14, STEAM_STAT_INT, "STAT_IRON_GUT" },
	{ 15, STEAM_STAT_INT, "STAT_BOTTLE_NOSED" },
	{ 16, STEAM_STAT_INT, "STAT_BARFIGHT_CHAMP" },
	{ 17, STEAM_STAT_INT, "STAT_VOLATILE" },
	{ 18, STEAM_STAT_INT, "STAT_SURROGATES" },
	{ 19, STEAM_STAT_INT, "STAT_KILL_COMMAND" },
	{ 20, STEAM_STAT_INT, "STAT_TRASH_COMPACTOR" },
	{ 21, STEAM_STAT_INT, "STAT_SPICY" },
	{ 22, STEAM_STAT_INT, "STAT_SERIAL_THRILLA" },
	{ 23, STEAM_STAT_INT, "STAT_TRADITION" },
	{ 24, STEAM_STAT_INT, "STAT_POP_QUIZ" },
	{ 25, STEAM_STAT_INT, "STAT_DYSLEXIA" },
	{ 26, STEAM_STAT_INT, "STAT_BOOKWORM" },
	{ 27, STEAM_STAT_INT, "STAT_MONARCH" },
	{ 28, STEAM_STAT_INT, "STAT_SUPER_SHREDDER" },
	{ 29, STEAM_STAT_INT, "STAT_FIXER_UPPER" },
	{ 30, STEAM_STAT_INT, "STAT_TORCHERER" },
	{ 31, STEAM_STAT_INT, "STAT_MANY_PEDI_PALP" },
	{ 32, STEAM_STAT_INT, "STAT_5000_SECOND_RULE" },
	{ 33, STEAM_STAT_INT, "STAT_SOCIAL_BUTTERFLY" },
	{ 34, STEAM_STAT_INT, "STAT_ROLL_THE_BONES" },
	{ 35, STEAM_STAT_INT, "STAT_COWBOY_FROM_HELL" },
	{ 36, STEAM_STAT_INT, "STAT_SELF_FLAGELLATION" },
	{ 37, STEAM_STAT_INT, "STAT_CHOPPING_BLOCK" },
	{ 38, STEAM_STAT_INT, "STAT_IF_YOU_LOVE_SOMETHING" },
	{ 39, STEAM_STAT_INT, "STAT_RAGE_AGAINST" },
	{ 40, STEAM_STAT_INT, "STAT_GUERILLA_RADIO" },
	{ 41, STEAM_STAT_INT, "STAT_FASCIST" },
	{ 42, STEAM_STAT_INT, "STAT_ITS_A_LIVING" },
	{ 43, STEAM_STAT_INT, "STAT_OVERCLOCKED" },
	{ 44, STEAM_STAT_INT, "STAT_BACK_TO_BASICS" },
	{ 45, STEAM_STAT_INT, "STAT_EXTRA_CREDIT" },
	{ 46, STEAM_STAT_INT, "STAT_EXTRA_CREDIT_LVLS" },
	{ 47, STEAM_STAT_INT, "STAT_DIPLOMA" },
	{ 48, STEAM_STAT_INT, "STAT_DIPLOMA_LVLS" },
	{ 49, STEAM_STAT_INT, "STAT_TUTORIAL_ENTERED" }
};

SteamStat_t g_SteamGlobalStats[NUM_GLOBAL_STEAM_STATISTICS] =
{
	{ 1, STEAM_STAT_INT,  "STAT_GLOBAL_GAMES_STARTED" },
	{ 2, STEAM_STAT_INT,  "STAT_GLOBAL_GAMES_WON" },
	{ 3, STEAM_STAT_INT,  "STAT_GLOBAL_BOULDER_DEATHS" },
	{ 4, STEAM_STAT_INT,  "STAT_GLOBAL_HERX_SLAIN" },
	{ 5, STEAM_STAT_INT,  "STAT_GLOBAL_BAPHOMET_SLAIN" },
	{ 6, STEAM_STAT_INT,  "STAT_GLOBAL_TWINSICE_SLAIN" },
	{ 7, STEAM_STAT_INT,  "STAT_GLOBAL_DEATHS_HUMAN" },
	{ 8, STEAM_STAT_INT,  "STAT_GLOBAL_DEATHS_RAT" },
	{ 9, STEAM_STAT_INT,  "STAT_GLOBAL_DEATHS_GOBLIN" },
	{ 10, STEAM_STAT_INT, "STAT_GLOBAL_DEATHS_SLIME" },
	{ 11, STEAM_STAT_INT, "STAT_GLOBAL_DEATHS_TROLL" },
	{ 12, STEAM_STAT_INT, "STAT_GLOBAL_DEATHS_SPIDER" },
	{ 13, STEAM_STAT_INT, "STAT_GLOBAL_DEATHS_GHOUL" },
	{ 14, STEAM_STAT_INT, "STAT_GLOBAL_DEATHS_SKELETON" },
	{ 15, STEAM_STAT_INT, "STAT_GLOBAL_DEATHS_SCORPION" },
	{ 16, STEAM_STAT_INT, "STAT_GLOBAL_DEATHS_IMP" },
	{ 17, STEAM_STAT_INT, "STAT_GLOBAL_DEATHS_GNOME" },
	{ 18, STEAM_STAT_INT, "STAT_GLOBAL_DEATHS_DEMON" },
	{ 19, STEAM_STAT_INT, "STAT_GLOBAL_DEATHS_SUCCUBUS" },
	{ 20, STEAM_STAT_INT, "STAT_GLOBAL_DEATHS_LICH" },
	{ 21, STEAM_STAT_INT, "STAT_GLOBAL_DEATHS_MINOTAUR" },
	{ 22, STEAM_STAT_INT, "STAT_GLOBAL_DEATHS_DEVIL" },
	{ 23, STEAM_STAT_INT, "STAT_GLOBAL_DEATHS_SHOPKEEPER" },
	{ 24, STEAM_STAT_INT, "STAT_GLOBAL_DEATHS_KOBOLD" },
	{ 25, STEAM_STAT_INT, "STAT_GLOBAL_DEATHS_SCARAB" },
	{ 26, STEAM_STAT_INT, "STAT_GLOBAL_DEATHS_CRYSTALGOLEM" },
	{ 27, STEAM_STAT_INT, "STAT_GLOBAL_DEATHS_INCUBUS" },
	{ 28, STEAM_STAT_INT, "STAT_GLOBAL_DEATHS_VAMPIRE" },
	{ 29, STEAM_STAT_INT, "STAT_GLOBAL_DEATHS_SHADOW" },
	{ 30, STEAM_STAT_INT, "STAT_GLOBAL_DEATHS_COCKATRICE" },
	{ 31, STEAM_STAT_INT, "STAT_GLOBAL_DEATHS_INSECTOID" },
	{ 32, STEAM_STAT_INT, "STAT_GLOBAL_DEATHS_GOATMAN" },
	{ 33, STEAM_STAT_INT, "STAT_GLOBAL_DEATHS_AUTOMATON" },
	{ 34, STEAM_STAT_INT, "STAT_GLOBAL_DEATHS_LICHICE" },
	{ 35, STEAM_STAT_INT, "STAT_GLOBAL_DEATHS_LICHFIRE" },
	{ 36, STEAM_STAT_INT, "STAT_GLOBAL_DEATHS_SENTRYBOT" },
	{ 37, STEAM_STAT_INT, "STAT_GLOBAL_DEATHS_SPELLBOT" },
	{ 38, STEAM_STAT_INT, "STAT_GLOBAL_DEATHS_GYROBOT" },
	{ 39, STEAM_STAT_INT, "STAT_GLOBAL_DEATHS_DUMMYBOT" },
	{ 40, STEAM_STAT_INT, "STAT_GLOBAL_TWINSFIRE_SLAIN" },
	{ 41, STEAM_STAT_INT, "STAT_GLOBAL_SHOPKEEPERS_SLAIN" },
	{ 42, STEAM_STAT_INT, "STAT_GLOBAL_MINOTAURS_SLAIN" },
	{ 43, STEAM_STAT_INT, "STAT_GLOBAL_TUTORIAL_ENTERED" },
	{ 44, STEAM_STAT_INT, "STAT_GLOBAL_TUTORIAL1_COMPLETED" },
	{ 45, STEAM_STAT_INT, "STAT_GLOBAL_TUTORIAL2_COMPLETED" },
	{ 46, STEAM_STAT_INT, "STAT_GLOBAL_TUTORIAL3_COMPLETED" },
	{ 47, STEAM_STAT_INT, "STAT_GLOBAL_TUTORIAL4_COMPLETED" },
	{ 48, STEAM_STAT_INT, "STAT_GLOBAL_TUTORIAL5_COMPLETED" },
	{ 49, STEAM_STAT_INT, "STAT_GLOBAL_TUTORIAL6_COMPLETED" },
	{ 50, STEAM_STAT_INT, "STAT_GLOBAL_TUTORIAL7_COMPLETED" },
	{ 51, STEAM_STAT_INT, "STAT_GLOBAL_TUTORIAL8_COMPLETED" },
	{ 52, STEAM_STAT_INT, "STAT_GLOBAL_TUTORIAL9_COMPLETED" },
	{ 53, STEAM_STAT_INT, "STAT_GLOBAL_TUTORIAL10_COMPLETED" },
	{ 54, STEAM_STAT_INT, "STAT_GLOBAL_TUTORIAL1_ATTEMPTS" },
	{ 55, STEAM_STAT_INT, "STAT_GLOBAL_TUTORIAL2_ATTEMPTS" },
	{ 56, STEAM_STAT_INT, "STAT_GLOBAL_TUTORIAL3_ATTEMPTS" },
	{ 57, STEAM_STAT_INT, "STAT_GLOBAL_TUTORIAL4_ATTEMPTS" },
	{ 58, STEAM_STAT_INT, "STAT_GLOBAL_TUTORIAL5_ATTEMPTS" },
	{ 59, STEAM_STAT_INT, "STAT_GLOBAL_TUTORIAL6_ATTEMPTS" },
	{ 60, STEAM_STAT_INT, "STAT_GLOBAL_TUTORIAL7_ATTEMPTS" },
	{ 61, STEAM_STAT_INT, "STAT_GLOBAL_TUTORIAL8_ATTEMPTS" },
	{ 62, STEAM_STAT_INT, "STAT_GLOBAL_TUTORIAL9_ATTEMPTS" },
	{ 63, STEAM_STAT_INT, "STAT_GLOBAL_TUTORIAL10_ATTEMPTS" },
	{ 64, STEAM_STAT_INT, "STAT_GLOBAL_DISABLE" },
	{ 65, STEAM_STAT_INT, "STAT_GLOBAL_PROMO" },
	{ 66, STEAM_STAT_INT, "STAT_GLOBAL_PROMO_INTERACT" }
};

#ifdef STEAMWORKS
bool directConnect = false;
CSteamLeaderboards* g_SteamLeaderboards = NULL;
CSteamWorkshop* g_SteamWorkshop = NULL;
CSteamStatistics* g_SteamStatistics = NULL;
#else
bool directConnect = true;
#endif
char address[64];
IPaddress net_server;
IPaddress* net_clients = nullptr;
UDPsocket net_sock = nullptr;
TCPsocket net_tcpsock = nullptr;
UDPpacket* net_packet = nullptr;
TCPsocket* net_tcpclients = nullptr;
SDLNet_SocketSet tcpset = nullptr;
list_t safePacketsSent;
std::unordered_map<int, Uint32> safePacketsReceivedMap[MAXPLAYERS];
bool receivedclientnum = false;
char const * window_title = nullptr;
bool softwaremode = false;
#ifdef NINTENDO
 std::chrono::time_point<std::chrono::steady_clock> lastTick;
#else
 SDL_TimerID timer;
#endif // NINTENDO
SDL_Window* screen = nullptr;
#ifdef APPLE
SDL_Renderer* renderer = nullptr;
#else
SDL_GLContext renderer;
#endif
SDL_Surface* mainsurface = nullptr;
SDL_Event event;
bool firstmouseevent = true;
int fullscreen = 0;
bool borderless = false;
bool smoothlighting = false;
list_t removedEntities;
list_t entitiesToDelete[MAXPLAYERS];
Entity* client_selected[MAXPLAYERS] = {nullptr, nullptr, nullptr, nullptr};
bool inrange[MAXPLAYERS];
Sint32 client_classes[MAXPLAYERS];
Uint32 client_keepalive[MAXPLAYERS];
Uint16 portnumber;
bool client_disconnected[MAXPLAYERS];
list_t entitiesdeleted;

// fps
bool showfps = false;
real_t t, ot = 0.0, frameval[AVERAGEFRAMES];
Uint32 cycles = 0, pingtime = 0;
Uint32 timesync = 0;
real_t fps = 0.0;

// world sim data
Sint32 camx = 0, camy = 0;
Sint32 ocamx = 0, ocamy = 0;
Sint32 newcamx, newcamy;
Uint32 entity_uids = 1, lastEntityUIDs = 1;
view_t cameras[MAXPLAYERS];
view_t menucam;
map_t map;
voxel_t** models = nullptr;
list_t button_l;
list_t light_l;
Uint32 mapseed;
bool* shoparea = nullptr;
real_t globalLightModifier = 0.f;
real_t globalLightTelepathyModifier = 0.f;
int globalLightSmoothingRate = 1;
int globalLightModifierActive = 0;

// game variables
Sint8 minimap[MINIMAP_MAX_DIMENSION][MINIMAP_MAX_DIMENSION];
bool loadnextlevel = false;
int skipLevelsOnLoad = 0;
bool loadingSameLevelAsCurrent = false;
std::string loadCustomNextMap = "";
Uint32 forceMapSeed = 0;
bool loading = false;
int currentlevel = 0, minotaurlevel = 0;
bool secretlevel = false;
bool darkmap = false;
bool skipintro = false;
bool broadcast = false;
bool nohud = false;
bool noclip = false, godmode = false, buddhamode = false;
bool everybodyfriendly = false;
bool combat = false, combattoggle = false;
bool assailant[MAXPLAYERS];
bool oassailant[MAXPLAYERS];
int assailantTimer[MAXPLAYERS] = { 0, 0, 0, 0 };
Uint32 nummonsters = 0;
bool gamePaused = false;
bool intro = true;
int introstage = -1;
bool movie = false;
int kills[NUMMONSTERS];

// messages
list_t messages;
list_t command_history;
node_t* chosen_command = nullptr;
bool command = false;
char command_str[128];

// editor variables
int drawlayer = OBSTACLELAYER, drawx = 0, drawy = 0, odrawx = 0, odrawy = 0;
int alllayers = 0;
int scroll = 0;
char layerstatus[20];
int menuVisible = 0;
int subwindow = 0;
int subx1, subx2, suby1, suby2;
char subtext[1024];
int toolbox = 1;
int statusbar = 1;
int viewsprites = 1;
int showgrid = 0;
int hovertext = 1;
int selectedTile = 0;
int tilepalette = 0;
int spritepalette = 0;
int mclick = 0;
int selectedTool = 0; // 0: Pencil 1: Point 2: Brush 3: Select 4: Fill
int allowediting = 0; // only turned on when the mouse is over paintable screen region
int openwindow = 0, savewindow = 0, newwindow = 0;
int slidery = 0, slidersize = 16;
int menuDisappear = 0;
int selectedFile = 0;
char** d_names = nullptr;
unsigned long d_names_length = 0;
char filename[128];
char foldername[128];
char oldfilename[128];
char message[256];
int messagetime = 0;
char widthtext[4], heighttext[4], nametext[32], authortext[32], skyboxtext[32];
char mapflagtext[MAPFLAGTEXTS][32];
char spriteProperties[32][128];
char tmpSpriteProperties[32][128];
int editproperty = 0;
SDL_Cursor* cursorArrow, *cursorPencil, *cursorPoint, *cursorBrush, *cursorSelect, *cursorFill;
int* palette;

// video definitions
polymodel_t* polymodels = nullptr;
bool useModelCache = false;
list_t ttfTextHash[HASH_SIZE];
TTF_Font* ttf8 = nullptr;
TTF_Font* ttf12 = nullptr;
TTF_Font* ttf16 = nullptr;
SDL_Surface* font8x8_bmp = nullptr;
SDL_Surface* font12x12_bmp = nullptr;
SDL_Surface* font16x16_bmp = nullptr;
SDL_Surface* fancyWindow_bmp = nullptr;
SDL_Surface** sprites = nullptr;
SDL_Surface** tiles = nullptr;
std::unordered_map<std::string, SDL_Surface*> achievementImages;
std::unordered_map<std::string, std::string> achievementNames;
std::unordered_map<std::string, std::string> achievementDesc;
std::unordered_set<std::string> achievementHidden;
std::set<std::pair<std::string, std::string>, Comparator> achievementNamesSorted;
std::unordered_map<std::string, int> achievementProgress; // ->second is the associated achievement stat index
std::unordered_map<std::string, int64_t> achievementUnlockTime;

std::unordered_set<std::string> achievementUnlockedLookup;
Uint32 imgref = 1, vboref = 1;
const Uint32 ttfTextCacheLimit = 9000;
GLuint* texid = nullptr;
bool disablevbos = false;
Uint32 fov = 65;
Uint32 fpsLimit = 60;
//GLuint *vboid=nullptr, *vaoid=nullptr;
SDL_Surface** allsurfaces;
Uint32 numsprites, numtiles, nummodels;
bool *animatedtiles = nullptr;
bool *lavatiles = nullptr;
bool *swimmingtiles = nullptr;
int rscale = 1;
real_t vidgamma = 1.0f;
real_t* zbuffer = nullptr;
Sint32* lightmap = nullptr;
Sint32* lightmapSmoothed = nullptr;
bool* vismap = nullptr;
bool mode3d = false;
bool verticalSync = false;
bool showStatusEffectIcons = true;
bool minimapPingMute = false;
bool mute_audio_on_focus_lost = false;
bool mute_player_monster_sounds = false;
int minimapTransparencyForeground = 0;
int minimapTransparencyBackground = 0;
int minimapScale = 4;
int minimapObjectZoom = 0;
int minimapScaleQuickToggle = 0;

// audio definitions
int audio_rate = 22050;
Uint16 audio_format = AUDIO_S16;
int audio_channels = 2;
int audio_buffers = 512;
int sfxvolume = 64;
int sfxAmbientVolume = 64;
int sfxEnvironmentVolume = 64;
int musvolume = 48;

// fun stuff
SDL_Surface* title_bmp = nullptr;
SDL_Surface* logo_bmp = nullptr;
SDL_Surface* cursor_bmp = nullptr;
SDL_Surface* cross_bmp = nullptr;
SDL_Surface* selected_cursor_bmp = nullptr;
SDL_Surface* controllerglyphs1_bmp = nullptr;
SDL_Surface* skillIcons_bmp = nullptr;
int shaking = 0, bobbing = 0;
bool fadeout = false, fadefinished = false;
int fadealpha = 0;
cameravars_t cameravars[MAXPLAYERS];

// misc definitions
char tempstr[1024];
char maptoload[256], configtoload[256];
bool loadingmap = false, genmap = false, loadingconfig = false;
bool deleteallbuttons = false;
Uint32 cursorflash = 0;

bool no_sound = false;

//Entity *players[4];

hit_t hit;

/*-------------------------------------------------------------------------------

	longestline

	returns the longest line of characters in a string (stopping for
	newlines)

-------------------------------------------------------------------------------*/

int longestline(char const * const str)
{
	int c, x = 0, result = 0;
	for ( c = 0; c < strlen(str); c++ )
	{
		if ( str[c] == 10 )
		{
			x = 0;
			continue;
		}
		x++;
		result = std::max(x, result);
	}
	return result;
}

/*-------------------------------------------------------------------------------

	concatedStringLength

	returns the length of all the given strings combined together
	e.g. concatedStringLength("chicken %s", "potato")

-------------------------------------------------------------------------------*/

int concatedStringLength(char* str, ...)
{
	va_list argptr;
	char newstr[1024] = { 0 };

	int result = 0;

	va_start(argptr, str);
	vsnprintf(newstr, 1023, str, argptr);
	va_end(argptr);

	return strlen(newstr);
}

/*-------------------------------------------------------------------------------

	sgn

	returns the sign of the given double (positive or negative);

-------------------------------------------------------------------------------*/

int sgn(real_t x)
{
	return (x > 0) - (x < 0);
}

/*-------------------------------------------------------------------------------

	numdigits

	return the number of digits of the given int (includes the sign if negative)

-------------------------------------------------------------------------------*/

int numdigits_sint16(Sint16 x)
{
	return snprintf(nullptr, 0, "%d", x);
}

/*-------------------------------------------------------------------------------

	printlog

	prints the given formatted text to the log file

-------------------------------------------------------------------------------*/

void printlog(const char* str, ...)
{
	char newstr[1024] = { 0 };
	va_list argptr;

	// format the content
	va_start( argptr, str );
	vsnprintf( newstr, 1023, str, argptr );
	va_end( argptr );

	// timestamp the message
	time_t timer;
	char buffer[32];
	struct tm* tm_info;
	time(&timer);
	tm_info = localtime(&timer);
	strftime( buffer, 32, "%H-%M-%S", tm_info );

	// print to the log
	if ( newstr[strlen(newstr) - 1] != '\n' )
	{
		int c = strlen(newstr);
		newstr[c] = '\n';
		newstr[c + 1] = 0;
	}
	fprintf( stderr, "[%s] %s", buffer, newstr );
	fflush( stderr );
}
