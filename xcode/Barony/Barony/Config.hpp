#define APPLE

#ifndef EDITOR
#define USE_FMOD
//#define STEAMWORKS
//#define USE_EOS
//#define BARONY_SUPER_MULTIPLAYER
#define USE_THEORA_VIDEO
#define GL_SILENCE_DEPRECATION
//#define USE_IMGUI
//#define USE_PLAYFAB
#endif

// defines tags necessary for USE_EOS,
// can be empty if USE_EOS not defined
#ifdef USE_EOS
#include "EOS_Config.hpp"
#endif

// defines tags necessary for USE_PLAYFAB
// can be empty if USE_PLAYFAB not defined
#ifdef USE_PLAYFAB
#include "PlayFab_Config.hpp"
#endif

#define EDITOR_EXE_NAME "editor"
#define BASE_DATA_DIR "./"
