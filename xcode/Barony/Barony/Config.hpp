#define APPLE

#ifndef EDITOR
#define USE_FMOD
#define STEAMWORKS
#define USE_EOS
#define USE_THEORA_PLAYER
//#define USE_IMGUI
#endif

// defines tags necessary for USE_EOS,
// can be empty if USE_EOS not defined
#include "EOS_Config.hpp"

#define EDITOR_EXE_NAME "editor"
#define BASE_DATA_DIR "./"
