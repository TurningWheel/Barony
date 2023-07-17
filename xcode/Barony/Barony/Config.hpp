#define APPLE

#ifndef EDITOR
#define USE_FMOD
//#define STEAMWORKS
//#define USE_EOS
//#define BARONY_SUPER_MULTIPLAYER
#define USE_THEORA_VIDEO
#define GL_SILENCE_DEPRECATION
//#define USE_IMGUI
#endif

// defines tags necessary for USE_EOS,
// can be empty if USE_EOS not defined
#ifdef USE_EOS
#include "EOS_Config.hpp"
#endif

#define EDITOR_EXE_NAME "editor"
#define BASE_DATA_DIR "./"
