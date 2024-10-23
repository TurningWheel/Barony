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

#ifdef WINDOWS
extern "C"
{
	__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

#include <assert.h>

char* stringCopy(char* dest, const char* src, size_t dest_size, size_t src_size) {
    // verify input
    assert(dest);
    assert(src);
    if (!dest || !src || !dest_size) {
	    return dest;
    }

    // copy string
    if (src_size < dest_size) {
        memcpy(dest, src, src_size);
        dest[src_size] = '\0';
    } else {
        memcpy(dest, src, dest_size);
        dest[dest_size - 1] = '\0';
    }

	return dest;
}

char* stringCopyUnsafe(char* dest, const char* src, size_t size) {
    // verify input
    assert(dest);
    assert(src);
    if (!dest || !src || !size) {
	    return dest;
    }

    // copy string
    --size;
    size_t c = 0;
    for (; c < size && src[c] != '\0'; ++c) {
        dest[c] = src[c];
    }
	dest[c] = '\0';

	return dest;
}

char* stringCat(char* dest, const char* src, size_t dest_size, size_t src_size) {
    // verify input
    assert(dest);
    assert(src);
    if (!dest || !src || !dest_size || !src_size) {
	    return dest;
    }

    // find end of dest string
    size_t off = 0;
    for (; off < dest_size && dest[off] != '\0'; ++off);
    dest += off;
    dest_size -= off;
    if (!dest || !dest_size) {
        return dest;
    }

    // copy string
    if (src_size < dest_size) {
        memcpy(dest, src, src_size);
        dest[src_size] = '\0';
    } else {
        memcpy(dest, src, dest_size);
        dest[dest_size - 1] = '\0';
    }

	return dest;
}

int stringCmp(const char* str1, const char* str2, size_t str1_size, size_t str2_size) {
    // verify input
    assert(str1);
    assert(str2);
    if (!str1 || !str2) {
	    return 0;
    }

    // scan strings for first difference
    size_t c = 0;
    for (; c < str1_size && c < str2_size &&
        str1[c] != '\0' && str2[c] != '\0' &&
        str1[c] == str2[c]; ++c);

    bool end_of_str1 = false;
    bool end_of_str2 = false;

    // reached end of first string'
    if (c == str1_size || str1[c] == '\0') {
        end_of_str1 = true;
    }

    // reached end of second string
    if (c == str2_size || str2[c] == '\0') {
        end_of_str2 = true;
    }

    if (end_of_str1 && end_of_str2) {
        return 0; // reached end of both strings, they are identical
    }
    else if (end_of_str1 && !end_of_str2) {
        return -str2[c]; // reached end of only str1, return <0
    }
    else if (end_of_str2 && !end_of_str1) {
        return str1[c]; // reached end of only str2, return >0
    }
    else {
        return str1[c] - str2[c]; // found a different character
    }
}

size_t stringLen(const char* str, size_t size) {
    // verify input
    assert(str);
    assert(size);
    if (!str || !size) {
	    return 0;
    }

    // find end of string
    size_t len = 0;
    for (; len < size && str[len] != '\0'; ++len);
    return len;
}

const char* stringStr(const char* str1, const char* str2, size_t str1_size, size_t str2_size) {
    // verify input
    assert(str1);
    assert(str2);
    if (!str1 || !str2) {
	    return nullptr;
    }

    // scan str1 for a match of str2
    for (size_t s = 0; s < str1_size && str1[s] != '\0'; ++s) {
        const char* ptr = str1 + s;
        if (!stringCmp(ptr, str2, str1_size - s, str2_size)) {
            return ptr;
        }
    }

    // no match found
    return nullptr;
}

char* stringStr(char* str1, const char* str2, size_t str1_size, size_t str2_size) {
    // verify input
    assert(str1);
    assert(str2);
    if (!str1 || !str2) {
	    return nullptr;
    }

    // scan str1 for a match of str2
    for (size_t s = 0; s < str1_size && str1[s] != '\0'; ++s) {
        char* ptr = str1 + s;
        if (!stringCmp(ptr, str2, str1_size - s, str2_size)) {
            return ptr;
        }
    }

    // no match found
    return nullptr;
}

#ifdef EDITOR
struct cvar_thingy {
    bool data = false;
} cvar_enableDebugKeys;
#else
#include "interface/consolecommand.hpp"
static ConsoleVariable<bool> cvar_enableDebugKeys("/enabledebugkeys", false, "if true, certain special keys can be used for debugging");
#endif

// main definitions
Sint32 display_id = 0;
#if defined(APPLE) && !defined(EDITOR)
// retina displays have higher DPI so we need a higher display resolution
Sint32 xres = 2560;
Sint32 yres = 1440;
#else
Sint32 xres = 1280;
Sint32 yres = 720;
#endif
int mainloop = 1;
bool initialized = false;
Uint32 ticks = 0;
bool stop = false;
SDL_bool EnableMouseCapture = SDL_TRUE; // disable if mouse capture causes problem debugging in Linux
bool& enableDebugKeys = cvar_enableDebugKeys.data;


// input stuff
Uint32 impulses[NUMIMPULSES];
Uint32 joyimpulses[NUM_JOY_IMPULSES];
Uint32 lastkeypressed = 0;
std::unordered_map<SDL_Keycode, bool> keystatus;
char* inputstr = nullptr;
int inputlen = 0;
bool fingerdown = false;
int fingerx = 0;
int fingery = 0;
int ofingerx = 0;
int ofingery = 0;
Sint8 mousestatus[6];
bool capture_mouse = true;
string lastname;

// net stuff
int numplayers = 0;
int clientnum = 0;
int multiplayer = 0;
SteamGlobalStat_t g_SteamAPIGlobalStats[1] =
{
	{ 57, STEAM_STAT_INT, "STAT_GLOBAL_GAME_STARTED" }
};

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
	{ 49, STEAM_STAT_INT, "STAT_TUTORIAL_ENTERED" },
	{ 50, STEAM_STAT_INT, "STAT_I_NEEDED_THAT" },
	{ 51, STEAM_STAT_INT, "STAT_DAPPER_1"},
	{ 52, STEAM_STAT_INT, "STAT_DAPPER_2"},
	{ 53, STEAM_STAT_INT, "STAT_DAPPER_3"},
	{ 54, STEAM_STAT_INT, "STAT_DAPPER"},
	{ 55, STEAM_STAT_INT, "STAT_DUNGEONSEED" },
	{ 56, STEAM_STAT_INT, "STAT_PITCH_PERFECT" },
	{ 57, STEAM_STAT_INT, "STAT_RUNG_OUT" },
	{ 58, STEAM_STAT_INT, "STAT_SMASH_MELEE" }
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
SDL_Window* screen = nullptr;
SDL_GLContext renderer = nullptr;
SDL_Event event;
bool firstmouseevent = true;
int fullscreen = 0;
bool borderless = false;
bool smoothlighting = true;
list_t removedEntities;
list_t entitiesToDelete[MAXPLAYERS];
Entity* client_selected[MAXPLAYERS] = {nullptr, nullptr, nullptr, nullptr};
bool inrange[MAXPLAYERS];
Sint32 client_classes[MAXPLAYERS];
Uint32 client_keepalive[MAXPLAYERS];
Uint16 portnumber;
bool client_disconnected[MAXPLAYERS] = { false };
list_t entitiesdeleted;

// fps
bool showfps = false;
real_t time_diff = 0.0;
real_t t, ot = 0.0, frameval[AVERAGEFRAMES];
Uint32 cycles = 0, pingtime = 0;
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
bool skipintro = true;
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
bool useModelCache = true;
list_t ttfTextHash[HASH_SIZE];
TTF_Font* ttf8 = nullptr;
TTF_Font* ttf12 = nullptr;
TTF_Font* ttf16 = nullptr;
SDL_Surface* font8x8_bmp = nullptr;
SDL_Surface* font12x12_bmp = nullptr;
SDL_Surface* font16x16_bmp = nullptr;
SDL_Surface** sprites = nullptr;
SDL_Surface** tiles = nullptr;

Uint32 imgref = 1, vboref = 1;
const Uint32 ttfTextCacheLimit = 9000;
GLuint* texid = nullptr;
bool disablevbos = false;
Uint32 fov = 60;
Uint32 fpsLimit = 60;
//GLuint *vboid=nullptr, *vaoid=nullptr;
SDL_Surface** allsurfaces;
Uint32 numsprites, numtiles, nummodels;
bool *animatedtiles = nullptr;
bool *lavatiles = nullptr;
bool *swimmingtiles = nullptr;
int rscale = 1;
real_t vidgamma = 1.0f;
std::vector<vec4_t> lightmaps[MAXPLAYERS + 1];
std::vector<vec4_t> lightmapsSmoothed[MAXPLAYERS + 1];
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
std::unordered_map<int, AnimatedTile> tileAnimations;

// audio definitions
int audio_rate = 22050;
Uint16 audio_format = AUDIO_S16;
int audio_channels = 2;
int audio_buffers = 512;
real_t sfxvolume = 1.0;
real_t sfxAmbientVolume = 1.0;
real_t sfxEnvironmentVolume = 1.0;
real_t sfxNotificationVolume = 1.0;
real_t musvolume = 1.0;
bool musicPreload = false;

// fun stuff
SDL_Surface* title_bmp = nullptr;
SDL_Surface* logo_bmp = nullptr;
SDL_Surface* cursor_bmp = nullptr;
SDL_Surface* cross_bmp = nullptr;
SDL_Surface* selected_cursor_bmp = nullptr;
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
bool splitscreen = false;

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
	char buffer[32];
    getTimeFormatted(getTime(), buffer, sizeof(buffer));

	// print to the log
	if ( newstr[strlen(newstr) - 1] != '\n' )
	{
		int c = (int)strlen(newstr);
		newstr[c] = '\n';
		newstr[c + 1] = 0;
	}
#ifndef NINTENDO
	//fprintf( stderr, "%s", newstr );
	fprintf( stderr, "[%s] %s", buffer, newstr );
	fflush( stderr );
#endif
	//fprintf( stdout, "%s", newstr );
	fprintf( stdout, "[%s] %s", buffer, newstr );
	fflush( stdout );
}

#ifdef NDEBUG
bool ENABLE_STACK_TRACES = false;
#else
bool ENABLE_STACK_TRACES = false;
#endif
static std::unordered_map<std::string, size_t> unique_traces;

#ifdef LINUX
#include <execinfo.h>
#endif

std::string stackTrace() {
#ifndef NDEBUG
    if (!ENABLE_STACK_TRACES) {
        return "";
    }
#ifdef LINUX

    // perform stack trace
    constexpr unsigned int STACK_SIZE = 16;
	void* array[STACK_SIZE];
	size_t size = backtrace(array, STACK_SIZE);
	if (size < 4) {
	    return "";
	}
	char** symbols = backtrace_symbols(array, size);

    // build string
    std::string trace;
	for (auto c = 3; c < size; ++c) {
	    trace += "\n";
	    symbols[c] = strrchr(symbols[c], (int)'(');
	    trace += symbols[c];
	}

	// free backtrace table
	free(symbols);

    return trace;
#endif
#endif
	return "";
}

void stackTraceUnique() {
#ifndef NDEBUG
    if (!ENABLE_STACK_TRACES) {
        return;
    }
#ifdef LINUX

    // perform stack trace
    constexpr unsigned int STACK_SIZE = 16;
	void* array[STACK_SIZE];
	size_t size = backtrace(array, STACK_SIZE);
	if (size < 4) {
	    return;
	}
	char** symbols = backtrace_symbols(array, size);

    // build string
    std::string trace;
	for (auto c = 3; c < size; ++c) {
	    trace += "\n";
	    //symbols[c] = strrchr(symbols[c], (int)'(');
	    trace += symbols[c];
	}

	// free backtrace table
	free(symbols);

    // attempt to place in map, or increment if it already exists
	auto result = unique_traces.emplace(trace, 1);
	if (result.second) {
	    // haven't seen this trace before
	    printlog(trace.c_str());
	} else {
	    // have seen this trace, simply increment counter
	    ++result.first->second;
	}
#endif
#endif
}

void finishStackTraceUnique() {
#ifndef NDEBUG
    if (!ENABLE_STACK_TRACES) {
        return;
    }
    printlog("Unique stack trace tally:");
    for (auto it = unique_traces.begin(); it != unique_traces.end(); ++it) {
        printlog(it->first.c_str());
        printlog("%llu", it->second);
    }
    unique_traces.clear();
#endif
}

#ifndef EDITOR
#include "interface/consolecommand.hpp"
static ConsoleCommand purgeStackTraces("/purge_stack_traces", "purge stack traces",
    [](int argc, const char* argv[]){
    finishStackTraceUnique();
    });
#endif

#ifdef NINTENDO
time_t getTime() {
    return nxGetTime();
}

char* getTimeFormatted(time_t t, char* buf, size_t size) {
    return nxGetTimeFormatted(t, buf, size);
}

char* getTimeAndDateFormatted(time_t t, char* buf, size_t size) {
    return nxGetTimeAndDateFormatted(t, buf, size);
}
#else // NINTENDO
time_t getTime() {
    return time(nullptr);
}

char* getTimeFormatted(time_t t, char* buf, size_t size) {
    struct tm* tm = localtime(&t);
    strftime(buf, size, "%H-%M-%S", tm);
    return buf;
}

char* getTimeAndDateFormatted(time_t t, char* buf, size_t size) {
    struct tm* tm = localtime(&t);
    strftime(buf, size, "%Y-%m-%d %H-%M-%S", tm);
    return buf;
}
#endif

void getTimeAndDate(time_t t, int* year, int* month, int* day, int* hour, int* min, int* second) {
    char buf[32];
    getTimeAndDateFormatted(t, buf, sizeof(buf));
    
    int _year, _month, _day, _hour, _min, _second;
    const int result = sscanf(buf, "%d-%d-%d %d-%d-%d", &_year, &_month, &_day, &_hour, &_min, &_second);
    assert(result == 6);
    
    if (year) { *year = _year; }
    if (month) { *month = _month; }
    if (day) { *day = _day; }
    if (hour) { *hour = _hour; }
    if (min) { *min = _min; }
    if (second) { *second = _second; }
}
