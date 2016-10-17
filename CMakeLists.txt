cmake_minimum_required(VERSION 2.6)
project(barony)

if (NOT CMAKE_BUILD_TYPE)
  set (CMAKE_BUILD_TYPE Release)
endif(NOT CMAKE_BUILD_TYPE)

if (DEFINED ENV{STEAMWORKS_ENABLED})
  set (STEAMWORKS_ENABLED $ENV{STEAMWORKS_ENABLED})
else()
  set (STEAMWORKS_ENABLED 0)
endif()

#FMOD can be disabled in one of two ways: 
# * Not having FMOD installed will automatically disable FMOD (and, consequently, the game's sound engine as a whole).
# * Running cmake with -DFMOD_ENABLED=OFF
set(FMOD 0) #Acts as an alias to FMOD_FOUND
option(FMOD_ENABLED "Use the FMOD Ex library in the sound engine" ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

set(CMAKE_OSX_ARCHITECTURES x86_64)

if (STEAMWORKS_ENABLED)
message("------------------------")
message("Building with steamworks")
message("------------------------")
else()
message("------------------------")
message("Building without steamworks")
message("------------------------")
endif()

set (Apple 0)
set (Windows 0)
set (Linux 0)
set (NOT_DWORD_DEFINED 0)
if( NOT WIN32 )
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall -pg -ffast-math -funroll-loops -fstrict-aliasing")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -pg")
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -ffast-math -funroll-loops -fstrict-aliasing")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
  if (APPLE)
    set (Apple 1)
    set (NOT_DWORD_DEFINED 1)
    #set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -x objective-c")
    #set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -x objective-c")
    set(MACOSX_BUNDLE true)
    set(MACOSX_BUNDLE_BUNDLE_NAME Barony)
    #TODO: Make version a define which is also set in src/Config.h
    set(MACOSX_BUNDLE_INFO_STRING Barony 2.0.2)
    set(MACOSX_BUNDLE_LONG_VERSION 2.0.2)
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING 2.0.2)
    set(MACOSX_BUNDLE_BUNDLE_VERSION 2.0.2)
    set(MACOSX_BUNDLE_ICON_FILE "barony.icns")
    #set(MACOSX_BUNDLE_ICON_FILE "game.icns")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -std=c++11 -stdlib=libc++")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -std=c++11 -stdlib=libc++")
  else()
    set (Apple 0)
    set (Linux 1) #TODO: Should not assume this? What if BSD/DOS/whatever-have-you?
    set (NOT_DWORD_DEFINED 1)
    if (NOT DISABLE_MALIGN_DOUBLE) #Only compile with -malign-double if not told to disable. Pass via command line flags.
      set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -malign-double")
      set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -malign-double")
    endif()
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -rdynamic -Wl,-rpath=$ORIGIN")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -std=c++11 -rdynamic -Wl,-rpath=$ORIGIN")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Wl,-rpath=$ORIGIN")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -std=c++11 -Wl,-rpath=$ORIGIN")
  endif()
else()
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall -pg -O3 -ffast-math -funroll-loops -malign-double -fstrict-aliasing -mwindows")
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -ffast-math -funroll-loops -malign-double -fstrict-aliasing -mwindows")
  set (Windows 1)
endif()

if (ARCHITECTURE)
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -mcpu=${ARCHITECTURE}")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_DEBUG} -mcpu=${ARCHITECTURE}")
endif()

if (APPLE)
  set ( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -LD_RUNPATH_SEARCH_PATHS=@loader_path/../Frameworks -rpath @loader_path/../Frameworks")

endif()
set ( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS}")
set ( CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS}")


IF(APPLE)
  #include_directories(/usr/X11R6/include/)
  #link_directories(/usr/X11R6/lib)
  #FIND_LIBRARY(OpenGL_LIBRARY OpenGL )
  #SET(EXTRA_LIBS ${OpenGL_LIBRARY} X11)
  FIND_LIBRARY(OpenGL_LIBRARY OpenGL )
  SET(EXTRA_LIBS ${OpenGL_LIBRARY})

  #set(CMAKE_LIBRARY_PATH "/Library/Frameworks/")
  find_package(SDL2 REQUIRED)
  find_package(SDL2_image REQUIRED)
  find_package(SDL2_net REQUIRED)
  find_package(SDL2_ttf REQUIRED)
ENDIF (APPLE)


# Find OpenGL
if (NOT APPLE)
  find_package(OpenGL REQUIRED)
endif()
#find_package(GLM REQUIRED)

if (NOT WIN32)
  if (NOT APPLE)
    INCLUDE(FindPkgConfig)
    PKG_SEARCH_MODULE(SDL2 REQUIRED sdl2)
    PKG_SEARCH_MODULE(SDL2IMAGE REQUIRED SDL2_image>=2.0.0)
    PKG_SEARCH_MODULE(SDL2NET REQUIRED SDL2_net)
    PKG_SEARCH_MODULE(SDL2TTF REQUIRED SDL2_ttf)
    find_package(SDL2_net REQUIRED)
    find_package(SDL2_ttf REQUIRED)
  endif()
endif()
if (NOT WIN32)
  INCLUDE_DIRECTORIES(${SDL2_INCLUDE_DIRS} ${SDL2_INCLUDE_DIR} ${SDL2IMAGE_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIR} ${SDL2NET_INCLUDE_DIR} ${SDL2_NET_INCLUDE_DIRS} ${SDL2TTF_INCLUDE_DIR}  ${SDL2_TTF_INCLUDE_DIRS})
else()
  INCLUDE_DIRECTORIES(${SDL2_INCLUDE_DIRS} ${SDL2IMAGE_INCLUDE_DIRS})
endif()

find_package(Threads REQUIRED)
if( NOT WIN32 AND NOT APPLE)
  #Linux needs these two.
  #Windows has manual links later on.
  #And Macs have their own special line.
  if (FMOD_ENABLED)
    find_package(FMOD)
    if (FMOD_FOUND)
      set(FMOD 1)
    endif()
  endif()
  find_package(PNG REQUIRED)
elseif (APPLE)
  #This is to make libpng work on Macs.
  exec_program(libpng-config ARGS --cflags
    OUTPUT_VARIABLE FMOD_CXX_FLAGS)
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FMOD_CXX_FLAGS}")
  MESSAGE("CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
  exec_program(libpng-config ARGS --ldflags
    OUTPUT_VARIABLE FMOD_LINK_FLAGS)
  set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${FMOD_LINK_FLAGS}")
  MESSAGE("CMAKE_EXE_LINKER_FLAGS: ${CMAKE_EXE_LINKER_FLAGS}")
endif()

if( NOT WIN32 AND STEAMWORKS_ENABLED)
  find_package(STEAMWORKS REQUIRED)
  #find_package(STEAMWORKS_CWRAPPER REQUIRED)
endif()
if (STEAMWORKS_ENABLED)
  #include_directories(${STEAMWORKS_CWRAPPER_INCLUDE_DIR})
  include_directories(${STEAMWORKS_INCLUDE_DIR})
endif()


# Add include directories
#include_directories(${GLM_INCLUDE_DIR})
if( NOT WIN32 )
  include_directories(${THREADS_INCLUDE_DIR})
endif()
if(NOT WIN32 AND NOT APPLE)
  #Remember, Windows and Mac don't have find_package for fmod and png.
  if (FMOD_FOUND)
    include_directories(${FMOD_INCLUDE_DIR})
  endif()
  include_directories(${PNG_INCLUDE_DIR})
elseif (APPLE)
  include_directories("${PROJECT_SOURCE_DIR}/libraries/fmod/inc")
  link_directories("${PROJECT_SOURCE_DIR}/libraries/fmod/lib")

  find_package(GLUT REQUIRED)
  include_directories(${GLUT_INCLUDE_DIR})
endif()

# Add the source directory
#file(GLOB_RECURSE SOURCE_FILES src/*.c src/*.h) # Can't do this because it'd then compile barony and editor together. Which just won't work.
#file(GLOB_RECURSE BOOK_FILES books/*)
#file(GLOB_RECURSE IMAGE_FILES images/*)
#file(GLOB_RECURSE ITEM_FILES items/*)
#file(GLOB_RECURSE MAP_FILES maps/*)
#file(GLOB_RECURSE MODEL_FILES models/*)
#file(GLOB_RECURSE MUSIC_FILES music/*)
#file(GLOB_RECURSE SOUND_FILES sound/*)

#add_subdirectory(books)

configure_file ( "${PROJECT_SOURCE_DIR}/src/Config.hpp.in" "${PROJECT_SOURCE_DIR}/src/Config.hpp")

message("***************************")
message("Debug flags: ")
message("${CMAKE_C_FLAGS_DEBUG}")
message("${CMAKE_CXX_FLAGS_DEBUG}")
#-g -Wall -pg -ffast-math -funroll-loops -fstrict-aliasing
message("Release flags: ")
message("${CMAKE_C_FLAGS_RELEASE}")
message("${CMAKE_CXX_FLAGS_RELEASE}")
#-O3 -DNDEBUG -Wall -pg -ffast-math -funroll-loops -fstrict-aliasing
message("***************************")


add_subdirectory(src)

#ADD_DEFINITONS(${PNG_DEFINITIONS})
if (APPLE)
set(COPY_FRAMEWORKS ${PROJECT_SOURCE_DIR}/../Frameworks/SDL.framework ${PROJECT_SOURCE_DIR}/../Frameworks/SDL_image.framework ${PROJECT_SOURCE_DIR}/../Frameworks/SDL_mixer.framework ${PROJECT_SOURCE_DIR}/../Frameworks/SDL_net.framework)
message("{COPY_FRAMEWORKS}: ${COPY_FRAMEWORKS}")
#add_executable(barony MACOSX_BUNDLE OSX/SDLmain.m ${GAME_SOURCES} ${COPY_FRAMEWORKS} ${BOOK_FILES} ${IMAGE_FILES} ${ITEM_FILES} ${MAP_FILES} ${MODEL_FILES} ${MUSIC_FILES} ${SOUND_FILES} libfmodex.dylib)
#add_executable(barony MACOSX_BUNDLE OSX/SDLmain.m ${GAME_SOURCES} ${COPY_FRAMEWORKS} libfmodex.dylib /opt/local/lib/libpng16.16.dylib)
#add_executable(barony MACOSX_BUNDLE OSX/SDLmain.m ${GAME_SOURCES} ${MACOSX_BUNDLE_ICON_FILE} libfmodex.dylib /opt/local/lib/libpng16.16.dylib)
add_executable(barony MACOSX_BUNDLE ${GAME_SOURCES} ${MACOSX_BUNDLE_ICON_FILE} libfmodex.dylib /opt/local/lib/libpng16.16.dylib)
#add_executable(barony OSX/SDLmain.m ${GAME_SOURCES})
#add_executable(barony ${GAME_SOURCES})
#SET_SOURCE_FILES_PROPERTIES(${COPY_FRAMEWORKS} PROPERTIES MACOSX_PACKAGE_LOCATION Frameworks)
#SET_SOURCE_FILES_PROPERTIES(${BOOK_FILES} PROPERTIES MACOSX_PACKAGE_LOCATION Resources/books/)
#SET_SOURCE_FILES_PROPERTIES(${IMAGE_FILES} PROPERTIES MACOSX_PACKAGE_LOCATION Resources/images/)
#SET_SOURCE_FILES_PROPERTIES(${ITEM_FILES} PROPERTIES MACOSX_PACKAGE_LOCATION Resources/items/)
#SET_SOURCE_FILES_PROPERTIES(${MAP_FILES} PROPERTIES MACOSX_PACKAGE_LOCATION Resources/maps/)
#SET_SOURCE_FILES_PROPERTIES(${MODEL_FILES} PROPERTIES MACOSX_PACKAGE_LOCATION Resources/models/)
#SET_SOURCE_FILES_PROPERTIES(${MUSIC_FILES} PROPERTIES MACOSX_PACKAGE_LOCATION Resources/music/)
#SET_SOURCE_FILES_PROPERTIES(${SOUND_FILES} PROPERTIES MACOSX_PACKAGE_LOCATION Resources/sound/)
SET_SOURCE_FILES_PROPERTIES(libfmodex.dylib PROPERTIES MACOSX_PACKAGE_LOCATION MacOS)
SET_SOURCE_FILES_PROPERTIES(/opt/local/lib/libpng16.16.dylib PROPERTIES MACOSX_PACKAGE_LOCATION MacOS)
SET_SOURCE_FILES_PROPERTIES(${COPY_RESOURCES} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
SET_SOURCE_FILES_PROPERTIES(${MACOSX_BUNDLE_ICON_FILE} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
set_source_files_properties(${GAME_SOURCES} PROPERTIES COMPILE_FLAGS "-x objective-c++")
#set_source_files_properties(${GAME_SOURCES} PROPERTIES COMPILE_FLAGS "-stdlib=libc++")
else()
add_executable(barony ${GAME_SOURCES})
endif()

if(WIN32)
  target_link_libraries(barony -lmingw32)
  target_link_libraries(barony -lSDL2main)
  target_link_libraries(barony -lSDL2)
  target_link_libraries(barony -lSDL2_net)
  target_link_libraries(barony -lSDL2_image)
  target_link_libraries(barony -lSDL2_ttf)
  if(STEAMWORKS_ENABLED)
    #target_link_libraries(barony -lsteamworks_cwrapper -lstdc++)
    target_link_libraries(barony -lsteam_api)
  endif()
  if (APPLE)
    target_link_libraries(barony ${GLUT_LIBRARIES})
  endif()
else()
  if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
    # 64 bit
    target_link_libraries(barony -L/usr/lib64 -lstdc++)
  endif()
  target_link_libraries(barony ${SDL2_LIBRARIES} ${SDL2_LIBRARY} ${SDL2IMAGE_LIBRARIES} ${SDL2_IMAGE_LIBRARIES} ${SDL2_NET_LIBRARIES} ${SDL2_TTF_LIBRARIES} ${SDL2TTF_LIBRARY})
  if (STEAMWORKS_ENABLED)
    target_link_libraries(barony ${STEAMWORKS_LIBRARY})
    #target_link_libraries(barony ${STEAMWORKS_CWRAPPER_LIBRARIES} -lstdc++)
  endif()
endif()
target_link_libraries(barony ${OPENGL_LIBRARIES})
target_link_libraries(barony ${THREADS_LIBRARIES})
target_link_libraries(barony -lm)
if( NOT WIN32 AND NOT APPLE)
  #Remember, Windows and Mac aren't using find_package for FMOD and PNG.
  target_link_libraries(barony ${PNG_LIBRARY})
  if (FMOD_FOUND)
    target_link_libraries(barony ${FMOD_LIBRARY})
  endif()
endif()
# We need to link to Winsock if we're on Windows
if(WIN32)
  target_link_libraries(barony wsock32 ws2_32)
  target_link_libraries(barony -lpng -lfmodex)
endif()
if(APPLE)
  target_link_libraries(barony -lfmodex) #Finally manually link fmod for mac.
endif()
target_link_libraries(barony ${EXTRA_LIBS}) #Apple needs this for OpenGL to work.

if (NOT APPLE)
#install(TARGETS barony
#  RUNTIME DESTINATION bin
#  COMPONENT Runtime
#)
else()
install(TARGETS barony
  RUNTIME DESTINATION bin COMPONENT Runtime BUNDLE DESTINATION Resources
)
endif()

if (APPLE)
  #add_executable(editor OSX/SDLmain.m ${EDITOR_SOURCES})
  #set(MACOSX_BUNDLE_ICON_FILE "${PROJECT_SOURCE_DIR}/editor.icns")
  #add_executable(editor MACOSX_BUNDLE OSX/SDLmain.m ${EDITOR_SOURCES} ${COPY_FRAMEWORKS} libfmodex.dylib  /opt/local/lib/libpng16.16.dylib)
  #add_executable(editor MACOSX_BUNDLE OSX/SDLmain.m ${EDITOR_SOURCES} libfmodex.dylib  /opt/local/lib/libpng16.16.dylib)
  add_executable(editor MACOSX_BUNDLE ${EDITOR_SOURCES} libfmodex.dylib  /opt/local/lib/libpng16.16.dylib)
  #SET_SOURCE_FILES_PROPERTIES(${MACOSX_BUNDLE_ICON_FILE} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
  set_source_files_properties(${EDITOR_SOURCES} PROPERTIES COMPILE_FLAGS "-x objective-c++")
else()
  add_executable(editor ${EDITOR_SOURCES})
endif()

if(WIN32)
  target_link_libraries(editor -lmingw32)
  target_link_libraries(editor -lSDL2main)
  target_link_libraries(editor -lSDL2)
  target_link_libraries(editor -lSDL2_net)
  target_link_libraries(editor -lSDL2_image)
  target_link_libraries(editor -lSDL2_ttf)
  if(STEAMWORKS_ENABLED)
    #target_link_libraries(editor -lsteamworks_cwrapper -lstdc++)
    target_link_libraries(editor -lsteam_api)
  endif()
else()
  if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
    # 64 bit
    target_link_libraries(editor -L/usr/lib64 -lstdc++)
  endif()
  target_link_libraries(editor ${SDL2_LIBRARIES} ${SDL2_LIBRARY} ${SDL2IMAGE_LIBRARIES} ${SDL2_IMAGE_LIBRARIES} ${SDL2_NET_LIBRARIES} ${SDL2_TTF_LIBRARIES} ${SDL2TTF_LIBRARY})
  if (STEAMWORKS_ENABLED)
    target_link_libraries(editor ${STEAMWORKS_LIBRARY})
    #target_link_libraries(editor ${STEAMWORKS_CWRAPPER_LIBRARIES} -lstdc++)
  endif()
endif()
target_link_libraries(editor ${OPENGL_LIBRARIES})
target_link_libraries(editor ${THREADS_LIBRARIES})
target_link_libraries(editor -lm)
# We need to link to Winsock if we're on Windows
if( NOT WIN32 AND NOT APPLE)
  #Remember, Windows and Mac aren't using find_package for FMOD and PNG.
  target_link_libraries(editor ${PNG_LIBRARY})
  if (FMOD_FOUND)
    target_link_libraries(editor ${FMOD_LIBRARY})
  endif()
endif()
if(WIN32)
  target_link_libraries(editor wsock32 ws2_32)
  target_link_libraries(editor -lfmodex)
endif()
if (APPLE)
  target_link_libraries(editor -lfmodex) #Finally manually link fmod for mac.
endif()
target_link_libraries(editor ${EXTRA_LIBS}) #Apple needs this for OpenGL to work.

#install(TARGETS editor
#  RUNTIME DESTINATION bin COMPONENT Runtime
#)

#install(DIRECTORY data DESTINATION share/barony )

if (APPLE)
#  SET_TARGET_PROPERTIES(
#    barony
#    PROPERTIES
#    RESOURCE ${BOOK_FILES} ${IMAGE_FILES} ${ITEM_FILES} ${MAP_FILES} ${MODEL_FILES} ${MUSIC_FILES} ${SOUND_FILES}
#  )
endif()
