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

// main definitions
Sint32 xres = 960;
Sint32 yres = 600;
int mainloop = 1;
bool initialized = FALSE;
Uint32 ticks = 0;
bool stop = FALSE;

// language stuff
char languageCode[32] = { 0 };
char **language = NULL;

// input stuff
int reversemouse=0;
double mousespeed=32;
Uint32 impulses[NUMIMPULSES];
Uint32 lastkeypressed=0;
Sint8 keystatus[512];
char *inputstr = NULL;
int inputlen=0;
Sint8 mousestatus[6];
Sint32 mousex=0, mousey=0;
Sint32 omousex=0, omousey=0;
Sint32 mousexrel=0, mouseyrel=0;
Entity *selectedEntity = NULL;
Entity **clickmap=NULL;
bool capture_mouse = TRUE;

// net stuff
Uint32 clientplayer=0;
int numplayers=0;
int clientnum=0;
int multiplayer=-1;
#ifdef STEAMWORKS
bool directConnect=FALSE;
#else
bool directConnect=TRUE;
#endif
char address[64];
IPaddress net_server;
IPaddress *net_clients=NULL;
UDPsocket net_sock=NULL;
TCPsocket net_tcpsock=NULL;
UDPpacket *net_packet=NULL;
TCPsocket *net_tcpclients=NULL;
SDLNet_SocketSet tcpset=NULL;
list_t safePacketsSent, safePacketsReceived[MAXPLAYERS];
bool receivedclientnum=FALSE;
char *window_title=NULL;
bool softwaremode=FALSE;
SDL_TimerID timer;
SDL_Window *screen=NULL;
#ifdef APPLE
SDL_Renderer *renderer=NULL;
#else
SDL_GLContext renderer;
#endif
SDL_Surface *mainsurface=NULL;
SDL_Event event;
bool firstmouseevent=TRUE;
int fullscreen = 0;
bool smoothlighting=FALSE;
list_t removedEntities;
list_t entitiesToDelete[MAXPLAYERS];
Entity *client_selected[MAXPLAYERS]={NULL,NULL,NULL,NULL};
bool inrange[MAXPLAYERS];
Sint32 client_classes[MAXPLAYERS];
Uint32 client_keepalive[MAXPLAYERS];
Uint32 portnumber;
bool client_disconnected[MAXPLAYERS];
list_t entitiesdeleted;

#ifdef STEAMWORKS
void *steamIDRemote[MAXPLAYERS]={NULL,NULL,NULL,NULL};
#endif

// fps
bool showfps=FALSE;
double t, ot = 0.0, frameval[AVERAGEFRAMES];
Uint32 cycles = 0, pingtime = 0;
Uint32 timesync = 0;
double fps = 0.0;

// world sim data
Sint32 camx=0, camy=0;
Sint32 ocamx=0, ocamy=0;
Sint32 newcamx, newcamy;
Uint32 entity_uids=1, lastEntityUIDs=1;
view_t camera;
map_t map;
voxel_t **models = NULL;
list_t button_l;
list_t light_l;
Uint32 mapseed;
bool *shoparea = NULL;

// game variables
Entity* openedChest[4] = { NULL };
bool shootmode=FALSE;
Sint8 minimap[64][64];
bool loadnextlevel=FALSE;
bool loading=FALSE;
int currentlevel=0, minotaurlevel=0;
bool secretlevel=FALSE;
bool darkmap=FALSE;
bool skipintro=FALSE;
bool broadcast=FALSE;
bool nohud=FALSE;
bool noclip=FALSE, godmode=FALSE, buddhamode=FALSE;
bool everybodyfriendly=FALSE;
bool combat=FALSE, combattoggle=FALSE;
bool assailant[MAXPLAYERS];
bool oassailant[MAXPLAYERS];
Uint32 nummonsters=0;
bool gamePaused=FALSE;
bool intro=TRUE;
int introstage=-1;
bool movie=FALSE;
int kills[NUMMONSTERS];

// messages
list_t messages;
list_t command_history;
node_t *chosen_command = NULL;
bool command=FALSE;
char command_str[128];

// editor variables
int drawlayer=OBSTACLELAYER, drawx=0, drawy=0, odrawx=0, odrawy=0;
int alllayers=0;
int scroll=0;
char layerstatus[20];
int menuVisible=0;
int subwindow=0;
int subx1, subx2, suby1, suby2;
char subtext[1024];
int toolbox=1;
int statusbar=1;
int viewsprites=1;
int showgrid=0;
int selectedTile=0;
int tilepalette=0;
int spritepalette=0;
int mclick=0;
int selectedTool=0; // 0: point draw 1: brush 2: select 3: fill
int allowediting=0; // only turned on when the mouse is over paintable screen region
int openwindow=0, savewindow=0, newwindow=0;
int slidery=0, slidersize=16;
int menuDisappear=0;
int selectedFile=0;
char **d_names=NULL;
unsigned long d_names_length=0;
char filename[128];
char oldfilename[128];
char message[48];
int messagetime=0;
char widthtext[4], heighttext[4], nametext[32], authortext[32];
int editproperty=0;
SDL_Cursor *cursorArrow, *cursorPencil, *cursorBrush, *cursorSelect, *cursorFill;
int *palette;

// video definitions
polymodel_t *polymodels=NULL;
list_t ttfTextHash[HASH_SIZE];
TTF_Font *ttf8=NULL;
TTF_Font *ttf12=NULL;
TTF_Font *ttf16=NULL;
SDL_Surface *font8x8_bmp=NULL;
SDL_Surface *font12x12_bmp=NULL;
SDL_Surface *font16x16_bmp=NULL;
SDL_Surface *fancyWindow_bmp=NULL;
SDL_Surface **sprites=NULL;
SDL_Surface **tiles=NULL;
Uint32 imgref=1, vboref=1;
GLuint *texid=NULL;
bool disablevbos=FALSE;
Uint32 fov=65;
//GLuint *vboid=NULL, *vaoid=NULL;
SDL_Surface **allsurfaces;
Uint32 numsprites, numtiles, nummodels;
bool *animatedtiles = NULL, *lavatiles = NULL;
int rscale = 1;
double vidgamma = 1.0f;
double *zbuffer = NULL;
Sint32 *lightmap=NULL;
bool *vismap=NULL;
bool mode3d=FALSE;

// audio definitions
int audio_rate = 22050;
Uint16 audio_format = AUDIO_S16;
int audio_channels = 2;
int audio_buffers = 512;
int sfxvolume=64;
int musvolume=48;

// fun stuff
SDL_Surface *title_bmp=NULL;
SDL_Surface *logo_bmp=NULL;
SDL_Surface *cursor_bmp=NULL;
SDL_Surface *cross_bmp=NULL;
int shaking=0, bobbing=0;
bool fadeout=FALSE, fadefinished=FALSE;
int fadealpha=0;
double camera_shakex;
double camera_shakex2;
int camera_shakey;
int camera_shakey2;

// misc definitions
char tempstr[1024];
char maptoload[256], configtoload[256];
bool loadingmap=FALSE, genmap=FALSE, loadingconfig=FALSE;
bool deleteallbuttons=FALSE;
Uint32 cursorflash=0;

bool no_sound = FALSE;

Entity *players[4];

hit_t hit;

/*-------------------------------------------------------------------------------

	longestline
	
	returns the longest line of characters in a string (stopping for
	newlines)

-------------------------------------------------------------------------------*/

int longestline(char *str) {
	int c, x=0, result=0;
	for( c=0; c<strlen(str); c++ ) {
		if( str[c]==10 ) {
			x=0;
			continue;
		}
		x++;
		result=std::max(x,result);
	}
	return result;
}

/*-------------------------------------------------------------------------------

	concatedStringLength

	returns the length of all the given strings combined together
	e.g. concatedStringLength("chicken %s", "potato")

-------------------------------------------------------------------------------*/

int concatedStringLength(char *str, ...)
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

int sgn(double x) {
	return (x>0)-(x<0);
}

/*-------------------------------------------------------------------------------

	numdigits

	return the number of digits of the given int (includes the sign if negative)

-------------------------------------------------------------------------------*/

int numdigits_sint16(Sint16 x)
{
	return snprintf(NULL, 0, "%d", x);
}

/*-------------------------------------------------------------------------------

	printlog

	prints the given formatted text to the log file

-------------------------------------------------------------------------------*/

void printlog(char *str, ...) {
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
	if( newstr[strlen(newstr)-1]!='\n' ) {
		int c = strlen(newstr);
		newstr[c] = '\n';
		newstr[c+1] = 0;
	}
	fprintf( stderr, "[%s] %s", buffer, newstr );
	fflush( stderr );
}