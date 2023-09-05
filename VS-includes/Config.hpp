
#define WINDOWS

//#define STEAMWORKS

#define USE_FMOD

#define EDITOR_EXE_NAME "editor"

#define BASE_DATA_DIR "./"

//#define DEBUG_ACHIEVEMENTS
//#define DEBUG_EVENT_TIMERS

#ifdef BARONY_DRM_FREE

#ifdef STEAMWORKS
#undef STEAMWORKS
#endif // STEAMWORKS

#endif
