/*-------------------------------------------------------------------------------

	BARONY
	File: main.hpp
	Desc: contains some prototypes as well as various type definitions

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#pragma once

#ifdef __arm__
typedef float real_t;
#else
typedef double real_t;
#endif

#include <cstdint>
#include <cstddef>
#include <algorithm>

// the following functions are safe variants of C's string library.
// they include the buffer length of each input as secondary parameters to
// prevent buffer overruns.
// they also ALWAYS append null when modifying a string, within the space
// provided.
// logically, if a function reaches the end of a string buffer as indicated
// by the given size, the behavior is identical to the case where it meets
// a null-terminator.
// in other words, input strings do not have to be null-terminated if their
// associated size argument matches exactly the amount of data you wish to use.
// otherwise the functions will stop at the first null-terminator found,
// matching the behavior of the original C library.

char* stringCopy(char* dest, const char* src, size_t dest_size, size_t src_size);
char* stringCopyUnsafe(char* dest, const char* src, size_t dest_size);
char* stringCat(char* dest, const char* src, size_t dest_size, size_t src_size);
int stringCmp(const char* str1, const char* str2, size_t str1_size, size_t str2_size);
size_t stringLen(const char* str, size_t size);
const char* stringStr(const char* str1, const char* str2, size_t str1_size, size_t str2_size);
char* stringStr(char* str1, const char* str2, size_t str1_size, size_t str2_size);

#include <iostream>
#include <list>
#include <string>
#include <vector>
#include <array>
//using namespace std; //For C++ strings //This breaks messages on certain systems, due to template<class _CharT> class std::__cxx11::messages
using std::string; //Instead of including an entire namespace, please explicitly include only the parts you need, and check for conflicts as reasonably possible.
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <functional>
#include "physfs.h"
#include "Config.hpp"

#ifdef NINTENDO
#include "nintendo/baronynx.hpp"
#endif

#ifdef STEAMWORKS
#define STEAM_APPID 371970
#endif

enum ESteamStatTypes
{
	STEAM_STAT_INT = 0,
	STEAM_STAT_FLOAT = 1,
	STEAM_STAT_AVGRATE = 2,
};

struct SteamStat_t
{
	int m_ID;
	ESteamStatTypes m_eStatType;
	const char *m_pchStatName;
	int m_iValue;
	float m_flValue;
	float m_flAvgNumerator;
	float m_flAvgDenominator;
};

extern bool spamming;
extern bool showfirst;
extern bool logCheckObstacle;
extern int logCheckObstacleCount;
extern bool logCheckMainLoopTimers;
extern bool autoLimbReload;

#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <fcntl.h>
#ifdef WINDOWS
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <unistd.h>
#include <limits.h>
#endif
#include <string.h>
#include <ctype.h>
#ifdef WINDOWS
#define GL_GLEXT_PROTOTYPES
#ifdef PATH_MAX
// replace with our own
#undef PATH_MAX
#endif
#define PATH_MAX 1024
#include <windows.h>
#pragma warning ( push )
#pragma warning( disable : 4091 ) // disable typedef warnings from dbghelp.h
#include <Dbghelp.h>
#pragma warning( pop )
#undef min
#undef max
#endif

#ifdef APPLE
 //#include <Cocoa/Cocoa.h>
 //#include <OpenGL/OpenGL.h>
 #define GL_GLEXT_PROTOTYPES
 #include <OpenGL/gl3ext.h>
 #include <OpenGL/gl3.h>
 #include <SDL2/SDL_opengl.h>
#else // APPLE
 #ifndef NINTENDO
  #define GL_GLEXT_PROTOTYPES
  #ifdef WINDOWS
    #include <GL/glew.h>
  #endif
  #include <GL/gl.h>
  #include <GL/glu.h>
  #ifdef LINUX
  	typedef uint16_t GLhalf;
  #endif
 #endif
#ifndef WINDOWS
 #include <GL/glext.h>
#endif
 #include "SDL_opengl.h"
#endif // !APPLE

#ifdef APPLE
#include <SDL2/SDL.h>
#else
#include "SDL.h"
#endif
#ifdef WINDOWS
#include "SDL_syswm.h"
#endif
#ifdef APPLE
 #include <SDL2_image/SDL_image.h>
#else // APPLE
 #ifndef NINTENDO
  #include "SDL_image.h"
 #endif // NINTENDO
#endif // !APPLE
#ifdef APPLE
#include <SDL2_net/SDL_net.h>
#else
#ifndef NINTENDO
#include "SDL_net.h"
#endif
#endif
#ifdef APPLE
#include <SDL2_ttf/SDL_ttf.h>
#else
#include "SDL_ttf.h"
#endif
//#include "sprig.h"
#include "savepng.hpp"

//Ifdef steam or something?
#ifdef STEAMWORKS
//#include <steamworks_cwrapper/steam_wrapper.h>
#endif

#ifdef WINDOWS
#include <io.h>
#define F_OK 0	// check for existence
#define X_OK 1	// check for execute permission
#define W_OK 2	// check for write permission
#define R_OK 4	// check for read permission

#if _MSC_VER != 1900 //Don't need this if running visual studio 2015.
#define snprintf _snprintf
#endif
#define access _access
#endif

#define PI 3.14159265358979323846

void printlog(const char* str, ...);
const char* gl_error_string(GLenum err);
#ifdef _MSC_VER
#define GL_CHECK_ERR(expression) expression;\
    {\
		GLenum err;\
		while((err = glGetError()) != GL_NO_ERROR) {\
			printlog("[OpenGL]: ERROR type = 0x%x, message = %s",\
				err, gl_error_string(err));\
		}\
	}
#define GL_CHECK_ERR_RET(expression) expression;\
    {\
		GLenum err;\
		while((err = glGetError()) != GL_NO_ERROR) {\
			printlog("[OpenGL]: ERROR type = 0x%x, message = %s",\
				err, gl_error_string(err));\
		}\
	}
#else
#define GL_CHECK_ERR(expression) ({ \
    expression;\
    GLenum err;\
    while((err = glGetError()) != GL_NO_ERROR) {\
        printlog("[OpenGL]: ERROR type = 0x%x, message = %s",\
            err, gl_error_string(err));\
    }\
})
#define GL_CHECK_ERR_RET(expression) ({ \
    auto retval = expression;\
    GLenum err;\
    while((err = glGetError()) != GL_NO_ERROR) {\
        printlog("[OpenGL]: ERROR type = 0x%x, message = %s",\
            err, gl_error_string(err));\
    }\
    retval;\
})
#endif

typedef struct vec4 {
    vec4(float f):
        x(f),
        y(f),
        z(f),
        w(f)
    {}
    vec4(float _x, float _y, float _z, float _w):
        x(_x),
        y(_y),
        z(_z),
        w(_w)
    {}
    vec4() = default;
    float x;
    float y;
    float z;
    float w;
} vec4_t;

typedef struct mat4x4 {
    mat4x4(float f):
        x(f, 0.f, 0.f, 0.f),
        y(0.f, f, 0.f, 0.f),
        z(0.f, 0.f, f, 0.f),
        w(0.f, 0.f, 0.f, f)
    {}
    mat4x4(
        float xx, float xy, float xz, float xw,
        float yx, float yy, float yz, float yw,
        float zx, float zy, float zz, float zw,
        float wx, float wy, float wz, float ww):
        x(xx, xy, xz, xw),
        y(yx, yy, yz, yw),
        z(zx, zy, zz, zw),
        w(wx, wy, wz, ww)
    {}
    mat4x4():
        mat4x4(1.f)
    {}
    vec4_t x;
    vec4_t y;
    vec4_t z;
    vec4_t w;
} mat4x4_t;

extern FILE* logfile;
extern SDL_bool EnableMouseCapture; // can disable this in main.cpp if mouse capture is causing problems with debugging on Linux
extern bool& enableDebugKeys; // if true, certain special keys can be used for debugging

class Item;
//enum Item;
//enum Status;

#define AVERAGEFRAMES 32

extern bool stop;

// impulses
#define IN_FORWARD 0
#define IN_LEFT 1
#define IN_BACK 2
#define IN_RIGHT 3
#define IN_TURNL 4
#define IN_TURNR 5
#define IN_UP 6
#define IN_DOWN 7
#define IN_CHAT 8
#define IN_COMMAND 9
#define IN_STATUS 10
#define IN_SPELL_LIST 11
#define IN_CAST_SPELL 12
#define IN_DEFEND 13
#define IN_ATTACK 14
#define IN_USE 15
#define IN_AUTOSORT 16
#define IN_MINIMAPSCALE 17
#define IN_TOGGLECHATLOG 18
#define IN_FOLLOWERMENU 19
#define IN_FOLLOWERMENU_LASTCMD 20
#define IN_FOLLOWERMENU_CYCLENEXT 21
#define IN_HOTBAR_SCROLL_LEFT 22
#define IN_HOTBAR_SCROLL_RIGHT 23
#define IN_HOTBAR_SCROLL_SELECT 24
#define NUMIMPULSES 25
static const std::vector<std::string> impulseStrings =
{
	"IN_FORWARD",
	"IN_LEFT",
	"IN_BACK",
	"IN_RIGHT",
	"IN_TURNL",
	"IN_TURNR",
	"IN_UP",
	"IN_DOWN",
	"IN_CHAT",
	"IN_COMMAND",
	"IN_STATUS",
	"IN_SPELL_LIST",
	"IN_CAST_SPELL",
	"IN_DEFEND",
	"IN_ATTACK",
	"IN_USE",
	"IN_AUTOSORT",
	"IN_MINIMAPSCALE",
	"IN_TOGGLECHATLOG",
	"IN_FOLLOWERMENU",
	"IN_FOLLOWERMENU_LASTCMD",
	"IN_FOLLOWERMENU_CYCLENEXT",
	"IN_HOTBAR_SCROLL_LEFT",
	"IN_HOTBAR_SCROLL_RIGHT",
	"IN_HOTBAR_SCROLL_SELECT"
};

//Joystick/gamepad impulses
//TODO: Split bindings into three subcategories: Bifunctional, Game Exclusive, Menu Exclusive.

//Bifunctional:
static const unsigned INJOY_STATUS = 0;
static const unsigned INJOY_SPELL_LIST = 1;
static const unsigned INJOY_PAUSE_MENU = 2; //Also acts as the back key/escape key in limited situations.
static const unsigned INJOY_DPAD_LEFT = 3;
static const unsigned INJOY_DPAD_RIGHT = 4;
static const unsigned INJOY_DPAD_UP = 5;
static const unsigned INJOY_DPAD_DOWN = 6;

//Menu Exclusive:
static const unsigned INJOY_MENU_LEFT_CLICK = 7;
static const unsigned INJOY_MENU_NEXT = 8;
static const unsigned INJOY_MENU_CANCEL = 9; //Basically the "b" button. Go back, cancel things, close dialogues...etc.
static const unsigned INJOY_MENU_SETTINGS_NEXT = 10;
static const unsigned INJOY_MENU_SETTINGS_PREV = 11; //TODO: Only one "cycle tabs" binding?
static const unsigned INJOY_MENU_REFRESH_LOBBY = 12;
static const unsigned INJOY_MENU_DONT_LOAD_SAVE = 13;
static const unsigned INJOY_MENU_RANDOM_NAME = 14;
static const unsigned INJOY_MENU_RANDOM_CHAR = 15; //Clears hotbar slot in-inventory.
static const unsigned INJOY_MENU_INVENTORY_TAB = 16; //Optimally, I'd like to just use one trigger to toggle between the two, but there's some issues with analog triggers.
static const unsigned INJOY_MENU_MAGIC_TAB = 17;
static const unsigned INJOY_MENU_USE = 18; //Opens the context menu in the inventory. Also grabs the highlighted item from a chest.
static const unsigned INJOY_MENU_HOTBAR_CLEAR = 19; //Clears hotbar slot in-inventory.
static const unsigned INJOY_MENU_DROP_ITEM = 20;
static const unsigned INJOY_MENU_CHEST_GRAB_ALL = 21;
static const unsigned INJOY_MENU_CYCLE_SHOP_LEFT = 22;
static const unsigned INJOY_MENU_CYCLE_SHOP_RIGHT = 23;
static const unsigned INJOY_MENU_BOOK_PREV = 24;
static const unsigned INJOY_MENU_BOOK_NEXT = 25;

static const unsigned INDEX_JOYBINDINGS_START_MENU = 7;

//Game Exclusive:
//These should not trigger if the in-game interfaces are brought up (!shootmode). Inventory, books, shops, chests, etc.
static const unsigned INJOY_GAME_USE = 26; //Used in-game for right click. NOTE: Not used in-inventory for in-world identification. Because clicking is disabled and whatnot. (Or can be done?)
static const unsigned INJOY_GAME_DEFEND = 27;
static const unsigned INJOY_GAME_ATTACK = 28;
static const unsigned INJOY_GAME_CAST_SPELL = 29;
static const unsigned INJOY_GAME_HOTBAR_ACTIVATE = 30; //Activates hotbar slot in-game.
static const unsigned INJOY_GAME_HOTBAR_PREV = 31;
static const unsigned INJOY_GAME_HOTBAR_NEXT = 32;
static const unsigned INJOY_GAME_MINIMAPSCALE = 33;
static const unsigned INJOY_GAME_TOGGLECHATLOG = 34;
static const unsigned INJOY_GAME_FOLLOWERMENU = 35;
static const unsigned INJOY_GAME_FOLLOWERMENU_LASTCMD = 36;
static const unsigned INJOY_GAME_FOLLOWERMENU_CYCLE = 37;

static const unsigned INDEX_JOYBINDINGS_START_GAME = 26;

static const unsigned NUM_JOY_IMPULSES = 38;

static const unsigned UNBOUND_JOYBINDING = 399;

static const int NUM_HOTBAR_CATEGORIES = 12; // number of filters for auto add hotbar items

static const int NUM_AUTOSORT_CATEGORIES = 12; // number of categories for autosort

static const int RIGHT_CLICK_IMPULSE = 285; // right click

// since SDL2 gets rid of these and we're too lazy to fix them...
#define SDL_BUTTON_WHEELUP 4
#define SDL_BUTTON_WHEELDOWN 5

//Time in seconds before the in_dev warning disappears.
#define indev_displaytime 7000

enum LightModifierValues : int
{
	GLOBAL_LIGHT_MODIFIER_STOPPED,
	GLOBAL_LIGHT_MODIFIER_INUSE,
	GLOBAL_LIGHT_MODIFIER_DISSIPATING
};

class Entity; //TODO: Bugger?

// node structure
typedef struct node_t
{
	struct node_t* next;
	struct node_t* prev;
	struct list_t* list;
	void* element;
	void (*deconstructor)(void* data);
	Uint32 size;
} node_t;

// list structure
typedef struct list_t
{
	node_t* first;
	node_t* last;
} list_t;
extern list_t button_l;
extern list_t light_l;

// game world structure
typedef struct map_t
{
	char name[32];   // name of the map
	char author[32]; // author of the map
	unsigned int width, height, skybox;  // size of the map + skybox
	Sint32 flags[16];
	Sint32* tiles = nullptr;
	std::unordered_map<Sint32, node_t*> entities_map;
	list_t* entities = nullptr;
	list_t* creatures = nullptr; //A list of Entity* pointers.
	list_t* worldUI = nullptr; //A list of Entity* pointers.
	bool* trapexcludelocations = nullptr;
	bool* monsterexcludelocations = nullptr;
	bool* lootexcludelocations = nullptr;
	std::set<int> liquidSfxPlayedTiles;
	char filename[256];
	~map_t()
	{
		if ( trapexcludelocations )
		{
			free(trapexcludelocations);
			trapexcludelocations = nullptr;
		}
		if ( monsterexcludelocations )
		{
			free(monsterexcludelocations);
			monsterexcludelocations = nullptr;
		}
		if ( lootexcludelocations )
		{
			free(lootexcludelocations);
			lootexcludelocations = nullptr;
		}
	}
} map_t;

#define MAPLAYERS 3 // number of layers contained in a single map
#define OBSTACLELAYER 1 // obstacle layer in map
#define MAPFLAGS 16 // map flags for custom properties
#define MAPFLAGTEXTS 20 // map flags for custom properties
// names for the flag indices
static const int MAP_FLAG_CEILINGTILE = 0;
static const int MAP_FLAG_DISABLETRAPS = 1;
static const int MAP_FLAG_DISABLEMONSTERS = 2;
static const int MAP_FLAG_DISABLELOOT = 3;
static const int MAP_FLAG_GENBYTES1 = 4;
static const int MAP_FLAG_GENBYTES2 = 5;
static const int MAP_FLAG_GENBYTES3 = 6;
static const int MAP_FLAG_GENBYTES4 = 7;
static const int MAP_FLAG_GENBYTES5 = 8;
static const int MAP_FLAG_GENBYTES6 = 9;
// indices for mapflagtext, 4 of these are stored as bytes within the above GENBYTES
static const int MAP_FLAG_GENTOTALMIN = 4;
static const int MAP_FLAG_GENTOTALMAX = 5;
static const int MAP_FLAG_GENMONSTERMIN = 6;
static const int MAP_FLAG_GENMONSTERMAX = 7;
static const int MAP_FLAG_GENLOOTMIN = 8;
static const int MAP_FLAG_GENLOOTMAX = 9;
static const int MAP_FLAG_GENDECORATIONMIN = 10;
static const int MAP_FLAG_GENDECORATIONMAX = 11;
static const int MAP_FLAG_DISABLEDIGGING = 12;
static const int MAP_FLAG_DISABLETELEPORT = 13;
static const int MAP_FLAG_DISABLELEVITATION = 14;
static const int MAP_FLAG_GENADJACENTROOMS = 15;
static const int MAP_FLAG_DISABLEOPENING = 16;
static const int MAP_FLAG_DISABLEMESSAGES = 17;
static const int MAP_FLAG_DISABLEHUNGER = 18;
static const int MAP_FLAG_PERIMETER_GAP = 19;

#define MFLAG_DISABLEDIGGING ((map.flags[MAP_FLAG_GENBYTES3] >> 24) & 0xFF) // first leftmost byte
#define MFLAG_DISABLETELEPORT ((map.flags[MAP_FLAG_GENBYTES3] >> 16) & 0xFF) // second leftmost byte
#define MFLAG_DISABLELEVITATION ((map.flags[MAP_FLAG_GENBYTES3] >> 8) & 0xFF) // third leftmost byte
#define MFLAG_GENADJACENTROOMS ((map.flags[MAP_FLAG_GENBYTES3] >> 0) & 0xFF) // fourth leftmost byte
#define MFLAG_DISABLEOPENING ((map.flags[MAP_FLAG_GENBYTES4] >> 24) & 0xFF) // first leftmost byte
#define MFLAG_DISABLEMESSAGES ((map.flags[MAP_FLAG_GENBYTES4] >> 16) & 0xFF) // second leftmost byte
#define MFLAG_DISABLEHUNGER ((map.flags[MAP_FLAG_GENBYTES4] >> 8) & 0xFF) // third leftmost byte
#define MFLAG_PERIMETER_GAP ((map.flags[MAP_FLAG_GENBYTES4] >> 0) & 0xFF) // fourth leftmost byte

// delete entity structure
typedef struct deleteent_t
{
	Uint32 uid;
	Uint32 tries;
} deleteent_t;
#define MAXTRIES 6 // max number of attempts on a packet
#define MAXDELETES 2 // max number of packets resent in a frame

// hit structure
#define HORIZONTAL 1
#define VERTICAL 2
typedef struct hit_t
{
	real_t x, y;
	int mapx, mapy;
	Entity* entity;
	int side;
} hit_t;
extern hit_t hit;

// button structure
typedef struct button_t
{
	char label[32];      // button label
	Sint32 x, y;         // onscreen position
	Uint32 sizex, sizey; // size of the button
	Uint8 visible;       // invisible buttons are ignored by the handler
	Uint8 focused;       // allows this button to function when a subwindow is open
	SDL_Keycode key;     // key shortcut to activate button
	int joykey;          // gamepad button used to activate this button.
	bool pressed;        // whether the button is being pressed or not
	bool needclick;      // involved in triggering buttons
	bool outline;        // draw golden border if true. For such things as indicated which settings tab gamepad has selected.

	// a pointer to the button's location in a list
	node_t* node;

	void (*action)(struct button_t* my);
} button_t;

// voxel structure
typedef struct voxel_t
{
	Sint32 sizex, sizey, sizez;
	Uint8* data;
	Uint8 palette[256][3];
} voxel_t;

// vertex structure
typedef struct vertex_t
{
	real_t x, y, z;
} vertex_t;

// quad structure
typedef struct polyquad_t
{
	vertex_t vertex[4];
	Uint8 r, g, b;
	int side;
} polyquad_t;

// triangle structure
typedef struct polytriangle_t
{
	vertex_t vertex[3];
    vertex_t normal;
	Uint8 r, g, b;
} polytriangle_t;

// polymodel structure
typedef struct polymodel_t
{
	polytriangle_t* faces;
	uint64_t numfaces;
    GLuint vao;
    
    // vbos
	GLuint positions;
	GLuint colors;
    GLuint normals;
	//GLuint colors_shifted;
	//GLuint grayscale_colors;
	//GLuint grayscale_colors_shifted;
} polymodel_t;

// string structure
typedef struct string_t
{
	Uint32 lines;
	char* data;
	node_t* node;
	Uint32 color;
	Uint32 time;
	int player = -1;
} string_t;

// door structure (used for map generation)
typedef struct door_t
{
	enum DoorDir : Sint32
	{
		DIR_EAST,
		DIR_SOUTH,
		DIR_WEST,
		DIR_NORTH
	};
	enum DoorEdge : Sint32
	{
		EDGE_EAST,
		EDGE_SOUTHEAST,
		EDGE_SOUTH,
		EDGE_SOUTHWEST,
		EDGE_WEST,
		EDGE_NORTHWEST,
		EDGE_NORTH,
		EDGE_NORTHEAST
	};
	Sint32 x, y;
	DoorDir dir;
	DoorEdge edge;
} door_t;

#define CLIPNEAR 2
#define CLIPFAR 4000
#define TEXTURESIZE 32
#define TEXTUREPOWER 5 // power of 2 that texture size is, ie pow(2,TEXTUREPOWER) = TEXTURESIZE
#ifdef BARONY_SUPER_MULTIPLAYER
#define MAXPLAYERS 8
#else
#define MAXPLAYERS 4
#endif

// shaking/bobbing, that sort of thing
struct cameravars_t {
	real_t shakex;
	real_t shakex2;
	int shakey;
	int shakey2;
};
extern cameravars_t cameravars[MAXPLAYERS];

extern int game;
extern bool loading;
extern SDL_Window* screen;
extern SDL_GLContext renderer;
extern SDL_Event event;
extern bool firstmouseevent;
extern char const * window_title;
extern Sint32 fullscreen;
extern bool borderless;
extern bool smoothlighting;
extern Sint32 display_id;
extern Sint32 xres;
extern Sint32 yres;
extern int mainloop;
extern Uint32 ticks;
extern Uint32 lastkeypressed;
extern std::unordered_map<SDL_Keycode, bool> keystatus;
extern Sint32 mousex, mousey;
extern Sint32 omousex, omousey;
extern Sint32 mousexrel, mouseyrel;
extern char* inputstr;
extern int inputlen;
extern string lastname;
extern bool fingerdown;
extern int fingerx;
extern int fingery;
extern int ofingerx;
extern int ofingery;
static const unsigned NUM_MOUSE_STATUS = 6;
extern Sint8 mousestatus[NUM_MOUSE_STATUS];
//extern Sint8 omousestatus[NUM_MOUSE_STATUS];
const int NUM_JOY_STATUS = SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_MAX;
//extern Sint8 joystatus[NUM_JOY_STATUS];
const int NUM_JOY_AXIS_STATUS = SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_MAX;
//extern Sint8 joy_trigger_status[NUM_JOY_TRIGGER_STATUS]; //0 = left, 1 = right.
extern Uint32 cursorflash;
extern Sint32 camx, camy;
extern Sint32 newcamx, newcamy;
extern int subwindow;
extern int subx1, subx2, suby1, suby2;
extern char subtext[1024];
extern int rscale;
extern real_t vidgamma;
extern bool verticalSync;
extern bool showStatusEffectIcons;
extern bool minimapPingMute;
extern bool mute_audio_on_focus_lost;
extern bool mute_player_monster_sounds;
extern int minimapTransparencyForeground;
extern int minimapTransparencyBackground;
extern int minimapScale;
extern int minimapObjectZoom;
extern std::vector<vec4_t> lightmaps[MAXPLAYERS + 1];
extern std::vector<vec4_t> lightmapsSmoothed[MAXPLAYERS + 1];
extern list_t entitiesdeleted;
extern Sint32 multiplayer;
extern bool directConnect;
extern bool client_disconnected[MAXPLAYERS];
extern int minotaurlevel;
#define SINGLE 0
#define SERVER 1
#define CLIENT 2
#define DIRECTSERVER 3
#define DIRECTCLIENT 4
#define SERVERCROSSPLAY 5
#define SPLITSCREEN 6

// language stuff
struct Language
{
	static const char* get(const int line);
	static std::map<int, std::string> entries;
	static std::map<int, std::string> tmpEntries;
	static void reset();
	static int loadLanguage(char const* const lang, bool forceLoadBaseDirectory);
	static int reloadLanguage();
	static std::string languageCode;
};

// random game defines
extern bool movie;
extern bool genmap;
extern char classtoquickstart[256];
extern bool splitscreen;

// commands
extern list_t messages;
extern list_t command_history;
extern node_t* chosen_command;
extern bool command;
extern char command_str[128];

// network definitions
extern IPaddress net_server;
extern IPaddress* net_clients;
extern UDPsocket net_sock;
extern UDPpacket* net_packet;
extern TCPsocket net_tcpsock;
extern TCPsocket* net_tcpclients;
extern SDLNet_SocketSet tcpset;

#define MAXTEXTURES 10240
#define MAXBUFFERS 256

#include "hash.hpp"

// various definitions
extern map_t map;
extern list_t ttfTextHash[HASH_SIZE];
extern TTF_Font* ttf8;
#define TTF8_WIDTH 7
#define TTF8_HEIGHT 12
extern TTF_Font* ttf12;
#define TTF12_WIDTH 9
#define TTF12_HEIGHT 16
extern TTF_Font* ttf16;
#define TTF16_WIDTH 12
#define TTF16_HEIGHT 22
extern SDL_Surface* font8x8_bmp;
extern SDL_Surface* font12x12_bmp;
extern SDL_Surface* font16x16_bmp;
extern SDL_Surface** sprites;
extern SDL_Surface** tiles;

extern voxel_t** models;
extern polymodel_t* polymodels;
extern bool useModelCache;
extern Uint32 imgref, vboref;
extern const Uint32 ttfTextCacheLimit;
extern GLuint* texid;
extern bool disablevbos;
extern Uint32 fov;
extern Uint32 fpsLimit;
//extern GLuint *vboid, *vaoid;
extern SDL_Surface** allsurfaces;
extern Uint32 numsprites;
extern Uint32 numtiles;
extern Uint32 nummodels;
extern Sint32 audio_rate, audio_channels, audio_buffers;
extern Uint16 audio_format;
extern real_t musvolume;
extern real_t sfxvolume;
extern real_t sfxAmbientVolume;
extern real_t sfxEnvironmentVolume;
extern real_t sfxNotificationVolume;
extern bool musicPreload;
extern bool *animatedtiles, *swimmingtiles, *lavatiles;
extern char tempstr[1024];
static const int MINIMAP_MAX_DIMENSION = 512;
extern Sint8 minimap[MINIMAP_MAX_DIMENSION][MINIMAP_MAX_DIMENSION];
extern Uint32 mapseed;
extern bool* shoparea;

struct AnimatedTile {
    int indices[8] = { 0 };
};
extern std::unordered_map<int, AnimatedTile> tileAnimations;

// function prototypes for main.c:
int sgn(real_t x);
int numdigits_sint16(Sint16 x);
int longestline(char const * const str);
int concatedStringLength(char* str, ...);

// function prototypes for list.c:
void list_FreeAll(list_t* list);
void list_RemoveNode(node_t* node);
template <typename T>
void list_RemoveNodeWithElement(list_t &list, T element)
{
	for ( node_t *node = list.first; node != nullptr; node = node->next )
	{
		if ( *static_cast<T*>(node->element) == element )
		{
			list_RemoveNode(node);
			return;
		}
	}
}
node_t* list_AddNodeFirst(list_t* list);
node_t* list_AddNodeLast(list_t* list);
node_t* list_AddNode(list_t* list, int index);
Uint32 list_Size(list_t* list);
list_t* list_Copy(list_t* destlist, list_t* srclist);
list_t* list_CopyNew(list_t* srclist);
Uint32 list_Index(node_t* node);
node_t* list_Node(list_t* list, int index);

// function prototypes for objects.c:
void defaultDeconstructor(void* data);
void emptyDeconstructor(void* data);
void entityDeconstructor(void* data);
void statDeconstructor(void* data);
void lightDeconstructor(void* data);
void mapDeconstructor(void* data);
void stringDeconstructor(void* data);
void listDeconstructor(void* data);
Entity* newEntity(Sint32 sprite, Uint32 pos, list_t* entlist, list_t* creaturelist);
button_t* newButton(void);
string_t* newString(list_t* list, Uint32 color, Uint32 time, int player, char const * const content, ...);

// function prototypes for cursors.c:
SDL_Cursor* newCursor(char const * const image[]);

// function prototypes for maps.c:
int generateDungeon(char* levelset, Uint32 seed, std::tuple<int, int, int, int> mapParameters = std::make_tuple(-1, -1, -1, 0)); // secretLevelChance of -1 is default Barony generation.
void assignActions(map_t* map);

// Cursor bitmap definitions
extern char const *cursor_pencil[];
extern char const *cursor_point[];
extern char const *cursor_brush[];
extern char const *cursor_fill[];

GLuint create_shader(const char* filename, GLenum type);

extern bool no_sound; //False means sound initialized properly. True means sound failed to initialize.
extern bool initialized; //So that messagePlayer doesn't explode before the game is initialized. //TODO: Does the editor need this set too and stuff?

void GO_SwapBuffers(SDL_Window* screen);

static const int NUM_STEAM_STATISTICS = 58;
extern SteamStat_t g_SteamStats[NUM_STEAM_STATISTICS];

#ifdef STEAMWORKS
 #include <steam/steam_api.h>
 struct SteamGlobalStat_t
 {
	 int m_ID;
	 ESteamStatTypes m_eStatType;
	 const char *m_pchStatName;
	 int64 m_iValue;
	 float m_flValue;
	 float m_flAvgNumerator;
	 float m_flAvgDenominator;
 };
 #include "steam.hpp"
 extern CSteamLeaderboards* g_SteamLeaderboards;
 extern CSteamWorkshop* g_SteamWorkshop;
 extern CSteamStatistics* g_SteamStatistics;
#else
struct SteamGlobalStat_t
{
	int m_ID;
	ESteamStatTypes m_eStatType;
	const char *m_pchStatName;

	long long m_iValue;
	float m_flValue;
	float m_flAvgNumerator;
	float m_flAvgDenominator;
};
#endif // STEAMWORKS
extern SteamGlobalStat_t g_SteamAPIGlobalStats[1];

#ifdef USE_EOS
 #include "eos.hpp"
#endif // USE_EOS

#ifndef NINTENDO
 #define getSizeOfText(A, B, C, D) TTF_SizeUTF8(A, B, C, D)
 #define getHeightOfFont(A) TTF_FontHeight(A)
#endif // NINTENDO

#if defined(NINTENDO) || (!defined(USE_EOS) && !defined(STEAMWORKS))
 #define LOCAL_ACHIEVEMENTS
#endif

std::string stackTrace();
void stackTraceUnique();
void finishStackTraceUnique();
extern bool ENABLE_STACK_TRACES;

time_t getTime();
void getTimeAndDate(time_t t, int* year, int* month, int* day, int* hour, int* min, int* second);
char* getTimeFormatted(time_t t, char* buf, size_t size);
char* getTimeAndDateFormatted(time_t t, char* buf, size_t size);

// I can't believe windows still defines these...
#ifdef far
#undef far
#endif
#ifdef near
#undef near
#endif

#define VERTEX_ARRAYS_ENABLED
