/*-------------------------------------------------------------------------------

	BARONY
	File: main.hpp
	Desc: contains some prototypes as well as various type definitions

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#pragma once

// undefine this if you don't want to build with steamworks features
//#define STEAMWORKS
//#undef STEAMWORKS //In case you don't want to use build scripts to disable steamworks.

#include <algorithm> //For min and max, because the #define breaks everything in c++.

#include <string>
//using namespace std; //For C++ strings //This breaks messages on certain systems, due to template<class _CharT> class std::__cxx11::messages
using std::string; //Instead of including an entire namespace, please explicitly include only the parts you need, and check for conflicts as reasonably possible.

#ifdef _WIN32
#define WINDOWS
#else
#include "Config.hpp"
#endif

#ifdef STEAMWORKS
#define STEAM_APPID 371970
#endif

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <fcntl.h>
#ifndef WINDOWS
	#include <unistd.h>
#endif
#include <string.h>
#include <ctype.h>
#ifdef WINDOWS
	#define GL_GLEXT_PROTOTYPES
	#include <windows.h>
	#undef min
	#undef max
#endif
#ifdef APPLE
	#include <Cocoa/Cocoa.h>
	//#include <OpenGL/OpenGL.h>
	#define GL_GLEXT_PROTOTYPES
	#include <GLUT/glut.h>
	#include <OpenGL/gl3ext.h>
	#include <OpenGL/gl3.h>
	#include <SDL2/SDL_opengl.h>
#else
	#define GL_GLEXT_PROTOTYPES
	#include <GL/gl.h>
	#include <GL/glu.h>
    #include <GL/glext.h>
    #include "SDL_opengl.h"
#endif
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
#else
    #include "SDL_image.h"
#endif
//#include "SDL_mixer.h"
#ifdef APPLE
    #include <SDL2_net/SDL_net.h>
#else
    #include "SDL_net.h"
#endif
#ifdef APPLE
    #include <SDL2_ttf/SDL_ttf.h>
#else
    #include "SDL_ttf.h"
#endif
//#include "sprig.h"
#include "savepng.hpp"
	
#ifndef APPLE
#define FALSE false
#define TRUE true
#endif

//Ifdef steam or something?
#ifdef STEAMWORKS
//#include <steamworks_cwrapper/steam_wrapper.h>
#endif

#ifdef _MSC_VER
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

extern FILE *logfile;

class Item;
//enum Item;
//enum Status;

#ifdef WINDOWS
extern PFNGLGENBUFFERSPROC SDL_glGenBuffers;
extern PFNGLBINDBUFFERPROC SDL_glBindBuffer;
extern PFNGLBUFFERDATAPROC SDL_glBufferData;
extern PFNGLDELETEBUFFERSPROC SDL_glDeleteBuffers;
extern PFNGLGENVERTEXARRAYSPROC SDL_glGenVertexArrays;
extern PFNGLBINDVERTEXARRAYPROC SDL_glBindVertexArray;
extern PFNGLDELETEVERTEXARRAYSPROC SDL_glDeleteVertexArrays;
extern PFNGLENABLEVERTEXATTRIBARRAYPROC SDL_glEnableVertexAttribArray;
extern PFNGLVERTEXATTRIBPOINTERPROC SDL_glVertexAttribPointer;
#else
#define SDL_glGenBuffers glGenBuffers
#define SDL_glBindBuffer glBindBuffer
#define SDL_glBufferData glBufferData
#define SDL_glDeleteBuffers glDeleteBuffers
#define SDL_glGenVertexArrays glGenVertexArrays
#define SDL_glBindVertexArray glBindVertexArray
#define SDL_glDeleteVertexArrays glDeleteVertexArrays
#define SDL_glEnableVertexAttribArray glEnableVertexAttribArray
#define SDL_glVertexAttribPointer glVertexAttribPointer
#endif

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
#define NUMIMPULSES 16

//Joystick/gamepad impulses
#define INJOY_STATUS 0
#define INJOY_SPELL_LIST 1
#define INJOY_CAST_SPELL 2
#define INJOY_DEFEND 3
#define INJOY_ATTACK 4
#define INJOY_USE 5
#define INJOY_PAUSE_MENU 6 //Should this also acts as the escape key?
#define INJOY_LEFT_CLICK 7
#define NUM_JOY_IMPULSES 8

// since SDL2 gets rid of these and we're too lazy to fix them...
#define SDL_BUTTON_WHEELUP 4
#define SDL_BUTTON_WHEELDOWN 5

//Time in seconds before the in_dev warning disappears.
#define indev_displaytime 7000

// view structure
typedef struct view_t {
	double x, y, z;
	double ang;
	double vang;
	Sint32 winx, winy, winw, winh;
} view_t;

extern view_t camera;

class Entity; //TODO: Bugger?

// node structure
typedef struct node_t {
	struct node_t *next;
	struct node_t *prev;
	struct list_t *list;
	void *element;
	void (*deconstructor)(void *data);
	Uint32 size;
} node_t;

// list structure
typedef struct list_t {
	node_t *first;
	node_t *last;
} list_t;
extern list_t button_l;
extern list_t light_l;

// game world structure
typedef struct map_t {
	char name[32];   // name of the map
	char author[32]; // author of the map
	unsigned int width, height;  // size of the map
	Sint32 *tiles;
	list_t *entities;
} map_t;

#define MAPLAYERS 3 // number of layers contained in a single map
#define OBSTACLELAYER 1 // obstacle layer in map

// light structure
typedef struct light_t {
	Sint32 x, y;
	Sint32 radius;
	Sint32 intensity;
	Sint32 *tiles;
	
	// a pointer to the light's location in a list
	node_t *node;
} light_t;

// delete entity structure
typedef struct deleteent_t {
	Uint32 uid;
	Uint32 tries;
} deleteent_t;
#define MAXTRIES 6 // max number of attempts on a packet
#define MAXDELETES 2 // max number of packets resent in a frame

// pathnode struct
typedef struct pathnode_t {
	struct pathnode_t *parent;
	Sint32 x, y;
	Uint32 g, h;
	node_t *node;
} pathnode_t;

// hit structure
#define HORIZONTAL 1
#define VERTICAL 2
typedef struct hit_t {
	double x, y;
	int mapx, mapy;
	Entity *entity;
	int side;
} hit_t;
extern hit_t hit;

// button structure
typedef struct button_t {
	char label[32];      // button label
	Sint32 x, y;         // onscreen position
	Uint32 sizex, sizey; // size of the button
	Uint8 visible;       // invisible buttons are ignored by the handler
	Uint8 focused;       // allows this button to function when a subwindow is open
	SDL_Keycode key;     // key shortcut to activate button
	bool pressed;        // whether the button is being pressed or not
	bool needclick;      // involved in triggering buttons
	
	// a pointer to the button's location in a list
	node_t *node;
	
	void (*action)(struct button_t *my);
} button_t;

// voxel structure
typedef struct voxel_t {
	Sint32 sizex, sizey, sizez;
	Uint8 *data;
	Uint8 palette[256][3];
} voxel_t;

// vertex structure
typedef struct vertex_t {
	double x, y, z;
} vertex_t;

// quad structure
typedef struct polyquad_t {
	vertex_t vertex[4];
	Uint8 r, g, b;
	int side;
} polyquad_t;

// triangle structure
typedef struct polytriangle_t {
	vertex_t vertex[3];
	Uint8 r, g, b;
} polytriangle_t;

// polymodel structure
typedef struct polymodel_t {
	polytriangle_t *faces;
	Uint32 numfaces;
	GLuint vbo;
	GLuint colors;
	GLuint colors_shifted;
	GLuint va;
} polymodel_t;

// string structure
typedef struct string_t {
	Uint32 lines;
	char *data;
	node_t *node;
	Uint32 color;
} string_t;

// door structure (used for map generation)
typedef struct door_t {
	Sint32 x, y;
	Sint32 dir; // 0: east, 1: south, 2: west, 3: north
} door_t;

#define CLIPNEAR 2
#define CLIPFAR 1024
#define TEXTURESIZE 32
#define TEXTUREPOWER 5 // power of 2 that texture size is, ie pow(2,TEXTUREPOWER) = TEXTURESIZE
#define MAXPLAYERS 4

extern int game;
extern bool loading;
extern SDL_TimerID timer;
extern SDL_Window *screen;
#ifdef APPLE
extern SDL_Renderer *renderer;
#else
extern SDL_GLContext renderer;
#endif
extern SDL_Surface *mainsurface;
extern SDL_Event event;
extern bool firstmouseevent;
extern char *window_title;
extern Sint32 fullscreen;
extern bool smoothlighting;
extern Sint32 xres;
extern Sint32 yres;
extern int mainloop;
extern Uint32 ticks;
extern Uint32 lastkeypressed;
extern Sint8 keystatus[512];
extern char *inputstr;
extern int inputlen;
extern string lastname;
extern Sint8 mousestatus[6];
const int NUM_JOY_STATUS = 32;
extern Sint8 joystatus[NUM_JOY_STATUS];
const int NUM_JOY_TRIGGER_STATUS = 2;
extern Sint8 joy_trigger_status[NUM_JOY_TRIGGER_STATUS]; //0 = left, 1 = right.
extern Uint32 cursorflash;
extern Sint32 camx, camy;
extern Sint32 newcamx, newcamy;
extern int subwindow;
extern int subx1, subx2, suby1, suby2;
extern char subtext[1024];
extern int rscale;
extern double vidgamma;
extern bool softwaremode;
extern double *zbuffer;
extern Sint32 *lightmap;
extern bool *vismap;
extern Entity **clickmap;
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

// language stuff
#define NUMLANGENTRIES 2000
extern char languageCode[32];
extern char **language;

// random game defines
extern bool movie;
extern bool genmap;
extern char classtoquickstart[256];

// commands
extern list_t messages;
extern list_t command_history;
extern node_t *chosen_command;
extern bool command;
extern char command_str[128];

// network definitions
extern IPaddress net_server;
extern IPaddress *net_clients;
extern UDPsocket net_sock;
extern UDPpacket *net_packet;
extern TCPsocket net_tcpsock;
extern TCPsocket *net_tcpclients;
extern SDLNet_SocketSet tcpset;

#ifdef STEAMWORKS
extern void *steamIDRemote[MAXPLAYERS]; //TODO: Bugger void pointer.
#endif

#define MAXTEXTURES 10240
#define MAXBUFFERS 256

#include "hash.hpp"

// various definitions
extern map_t map;
extern list_t ttfTextHash[HASH_SIZE];
extern TTF_Font *ttf8;
#define TTF8_WIDTH 7
#define TTF8_HEIGHT 12
extern TTF_Font *ttf12;
#define TTF12_WIDTH 9
#define TTF12_HEIGHT 16
extern TTF_Font *ttf16;
#define TTF16_WIDTH 12
#define TTF16_HEIGHT 22
extern SDL_Surface *font8x8_bmp;
extern SDL_Surface *font12x12_bmp;
extern SDL_Surface *font16x16_bmp;
extern SDL_Surface *fancyWindow_bmp;
extern SDL_Surface **sprites;
extern SDL_Surface **tiles;
extern voxel_t **models;
extern polymodel_t *polymodels;
extern Uint32 imgref, vboref;
extern GLuint *texid;
extern bool disablevbos;
extern Uint32 fov;
//extern GLuint *vboid, *vaoid;
extern SDL_Surface **allsurfaces;
extern Uint32 numsprites;
extern Uint32 numtiles;
extern Uint32 nummodels;
extern Sint32 audio_rate, audio_channels, audio_buffers;
extern Uint16 audio_format;
extern int sfxvolume;
extern bool *animatedtiles, *lavatiles;
extern char tempstr[1024];
extern Sint8 minimap[64][64];
extern Uint32 mapseed;
extern bool *shoparea;

// function prototypes for main.c:
int sgn(double x);
int numdigits_sint16(Sint16 x);
int longestline(char *str);
int concatedStringLength(char *str, ...);
void printlog(char *str, ...);

// function prototypes for init.c:
int initApp(char *title, int fullscreen);
int deinitApp();
bool initVideo();
bool changeVideoMode();
void generatePolyModels();
void generateVBOs();
int loadLanguage(char *lang);
int reloadLanguage();

// function prototypes for list.c:
void list_FreeAll(list_t *list);
void list_RemoveNode(node_t *node);
node_t *list_AddNodeFirst(list_t *list);
node_t *list_AddNodeLast(list_t *list);
node_t *list_AddNode(list_t *list, int index);
Uint32 list_Size(list_t *list);
list_t *list_Copy(list_t *destlist, list_t *srclist);
list_t *list_CopyNew(list_t *srclist);
Uint32 list_Index(node_t *node);
node_t *list_Node(list_t *list, int index);

// function prototypes for light.c:
light_t *lightSphereShadow(Sint32 x, Sint32 y, Sint32 radius, Sint32 intensity);
light_t *lightSphere(Sint32 x, Sint32 y, Sint32 radius, Sint32 intensity);

// function prototypes for objects.c:
void defaultDeconstructor(void *data);
void emptyDeconstructor(void *data);
void entityDeconstructor(void *data);
void lightDeconstructor(void *data);
void mapDeconstructor(void *data);
void stringDeconstructor(void *data);
void listDeconstructor(void *data);
Entity *newEntity(Sint32 sprite, Uint32 pos, list_t *entlist);
button_t *newButton(void);
light_t *newLight(Sint32 x, Sint32 y, Sint32 radius, Sint32 intensity);
string_t *newString(list_t *list, Uint32 color, char *content,...);
pathnode_t *newPathnode(list_t *list, Sint32 x, Sint32 y, pathnode_t *parent, Sint8 pos);

// function prototypes for draw.c:
#define FLIP_VERTICAL 1
#define FLIP_HORIZONTAL 2
SDL_Surface *flipSurface(SDL_Surface *surface, int flags);
void drawCircle(int x, int y, double radius, Uint32 color, Uint8 alpha);
void drawArc(int x, int y, double radius, double angle1, double angle2, Uint32 color, Uint8 alpha);
void drawLine(int x1, int y1, int x2, int y2, Uint32 color, Uint8 alpha);
int drawRect(SDL_Rect *src, Uint32 color, Uint8 alpha);
void drawGear(Sint16 x, Sint16 y, double size, Sint32 rotation);
void drawImage(SDL_Surface *image, SDL_Rect *src, SDL_Rect *pos);
void drawImageScaled(SDL_Surface *image, SDL_Rect *src, SDL_Rect *pos);
void drawImageAlpha(SDL_Surface *image, SDL_Rect *src, SDL_Rect *pos, Uint8 alpha);
void drawImageColor(SDL_Surface *image, SDL_Rect *src, SDL_Rect *pos, Uint32 color);
void drawImageFancy(SDL_Surface *image, Uint32 color, double angle, SDL_Rect *src, SDL_Rect *pos);
void drawImageRotatedAlpha(SDL_Surface *image, SDL_Rect *src, SDL_Rect *pos, double angle, Uint8 alpha);
SDL_Surface* scaleSurface(SDL_Surface *Surface, Uint16 Width, Uint16 Height);
void drawSky3D(view_t *camera, SDL_Surface *tex);
void drawLayer(long camx, long camy, int z, map_t *map);
void drawBackground(long camx, long camy);
void drawForeground(long camx, long camy);
void drawClearBuffers();
void raycast(view_t *camera, int mode);
void drawFloors(view_t *camera);
void drawSky(SDL_Surface *srfc);
void drawVoxel(view_t *camera, Entity *entity);
void drawEntities3D(view_t *camera, int mode);
void drawPalette(voxel_t *model);
void drawEntities2D(long camx, long camy);
void drawGrid(long camx, long camy);
void drawEditormap(long camx, long camy);
void drawWindow(int x1, int y1, int x2, int y2);
void drawDepressed(int x1, int y1, int x2, int y2);
void drawWindowFancy(int x1, int y1, int x2, int y2);
SDL_Rect ttfPrintTextColor( TTF_Font *font, int x, int y, Uint32 color, bool outline, const char *str );
SDL_Rect ttfPrintText( TTF_Font *font, int x, int y, const char *str );
SDL_Rect ttfPrintTextFormattedColor( TTF_Font *font, int x, int y, Uint32 color, char *fmt, ... );
SDL_Rect ttfPrintTextFormatted( TTF_Font *font, int x, int y, char *fmt, ... );
void printTextFormatted( SDL_Surface *font_bmp, int x, int y, char *fmt, ... );
void printTextFormattedAlpha(SDL_Surface *font_bmp, int x, int y, Uint8 alpha, char *fmt, ...);
void printTextFormattedColor(SDL_Surface *font_bmp, int x, int y, Uint32 color, char *fmt, ...);
void printTextFormattedFancy(SDL_Surface *font_bmp, int x, int y, Uint32 color, double angle, double scale, char *fmt, ...);
void printText( SDL_Surface *font_bmp, int x, int y, char *str );
void drawSprite(view_t *camera, Entity *entity);
void drawTooltip(SDL_Rect *src);

// function prototypes for opengl.c:
#define REALCOLORS 0
#define ENTITYUIDS 1
double getLightForEntity(double x, double y);
void glDrawVoxel(view_t *camera, Entity *entity, int mode);
void glDrawSprite(view_t *camera, Entity *entity, int mode);
double getLightAt(int x, int y);
void glDrawWorld(view_t *camera, int mode);

// function prototypes for files.c:
void glLoadTexture(SDL_Surface *image, int texnum);
SDL_Surface *loadImage(char *filename);
voxel_t *loadVoxel(char *filename2);
int loadMap(char *filename, map_t *destmap, list_t *entlist);
int loadConfig(char *filename);
int saveMap(char *filename);

// function prototypes for cursors.c:
SDL_Cursor *newCursor(char *image[]);

// function prototypes for maps.c:
int generateDungeon(char *levelset, Uint32 seed);
void assignActions(map_t *map);

// cursor bitmap definitions
extern char *cursor_pencil[];
extern char *cursor_brush[];
extern char *cursor_fill[];

GLuint create_shader(const char* filename, GLenum type);

char *readFile(char *filename);
list_t *directoryContents(char *directory);

extern bool no_sound; //False means sound initialized properly. True means sound failed to initialize.
extern bool initialized; //So that messagePlayer doesn't explode before the game is initialized. //TODO: Does the editor need this set too and stuff?
