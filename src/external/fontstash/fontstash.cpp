
//This file is a stub to build the Fontstash implementation.

#include <stdio.h>
#define FONTSTASH_IMPLEMENTATION	// Expands implementation
#include "fontstash.h"

#ifdef NINTENDO
 #include "../../nintendo/baronynx.hpp"
#else
 #ifdef WINDOWS
  #include <SDL_opengl.h> //TODO: Make this uniform across all platforms?
 #else
  #include <SDL2/SDL_opengl.h>
 #endif
#endif

#define GLFONTSTASH_IMPLEMENTATION	// Expands implementation
//#define FONTSTASH_IMPLEMENTATION
#include "glfontstash.h"
