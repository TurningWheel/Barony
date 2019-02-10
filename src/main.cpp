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
Sint8 joystatus[NUM_JOY_STATUS];
Sint8 joy_trigger_status[NUM_JOY_TRIGGER_STATUS];
Entity** clickmap = nullptr;
bool capture_mouse = true;
string lastname;
int lastCreatedCharacterClass = -1;
int lastCreatedCharacterAppearance = -1;
int lastCreatedCharacterSex = -1;

// net stuff
Uint32 clientplayer = 0;
int numplayers = 0;
int clientnum = 0;
int multiplayer = -1;
#ifdef STEAMWORKS
bool directConnect = false;
CSteamLeaderboards* g_SteamLeaderboards = NULL;
CSteamWorkshop* g_SteamWorkshop = NULL;
SteamStat_t g_SteamStats[NUM_STEAM_STATISTICS] =
{
	{1, STEAM_STAT_INT, "STAT_BOULDER_DEATHS"},
	{2, STEAM_STAT_INT, "STAT_WORTHLESS_GLASS"},
	{3, STEAM_STAT_INT, "STAT_TOUGH_AS_NAILS"},
	{4, STEAM_STAT_INT, "STAT_UNSTOPPABLE_FORCE"},
	{5, STEAM_STAT_INT, "STAT_GAMES_STARTED"},
	{6, STEAM_STAT_INT, "STAT_GAMES_WON"}
};
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
list_t safePacketsSent, safePacketsReceived[MAXPLAYERS];
bool receivedclientnum = false;
char* window_title = nullptr;
bool softwaremode = false;
SDL_TimerID timer;
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
view_t camera;
map_t map;
voxel_t** models = nullptr;
list_t button_l;
list_t light_l;
Uint32 mapseed;
bool* shoparea = nullptr;

// game variables
bool shootmode = false;
Sint8 minimap[64][64];
bool loadnextlevel = false;
int skipLevelsOnLoad = 0;
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
Uint32 imgref = 1, vboref = 1;
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
bool* vismap = nullptr;
bool mode3d = false;
bool verticalSync = false;
bool showStatusEffectIcons = true;
bool minimapPingMute = false;
bool mute_audio_on_focus_lost = false;
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
int musvolume = 48;

// fun stuff
SDL_Surface* title_bmp = nullptr;
SDL_Surface* logo_bmp = nullptr;
SDL_Surface* cursor_bmp = nullptr;
SDL_Surface* cross_bmp = nullptr;
int shaking = 0, bobbing = 0;
bool fadeout = false, fadefinished = false;
int fadealpha = 0;
real_t camera_shakex;
real_t camera_shakex2;
int camera_shakey;
int camera_shakey2;

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

int longestline(char* str)
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
