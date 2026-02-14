cmake_minimum_required(VERSION 3.16)
project(theoraplayer_builder LANGUAGES C CXX)

if (NOT DEFINED THEORAPLAYER_ROOT OR "${THEORAPLAYER_ROOT}" STREQUAL "")
  message(FATAL_ERROR "THEORAPLAYER_ROOT must be provided.")
endif()

set(THEORAPLAYER_DEP_BACKEND "pkgconfig" CACHE STRING "TheoraPlayer dependency backend (pkgconfig or vcpkg)")
set_property(CACHE THEORAPLAYER_DEP_BACKEND PROPERTY STRINGS pkgconfig vcpkg)

if (THEORAPLAYER_DEP_BACKEND STREQUAL "pkgconfig")
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(OGG REQUIRED IMPORTED_TARGET ogg)
  pkg_check_modules(VORBIS REQUIRED IMPORTED_TARGET vorbis)
  pkg_check_modules(VORBISFILE REQUIRED IMPORTED_TARGET vorbisfile)
  pkg_check_modules(VORBISENC REQUIRED IMPORTED_TARGET vorbisenc)
  pkg_check_modules(THEORA REQUIRED IMPORTED_TARGET theora)
  pkg_check_modules(THEORADEC REQUIRED IMPORTED_TARGET theoradec)
  pkg_check_modules(THEORAENC REQUIRED IMPORTED_TARGET theoraenc)
  set(THEORAPLAYER_DEP_LIBS
    PkgConfig::OGG
    PkgConfig::VORBIS
    PkgConfig::VORBISFILE
    PkgConfig::VORBISENC
    PkgConfig::THEORA
    PkgConfig::THEORADEC
    PkgConfig::THEORAENC
  )
elseif (THEORAPLAYER_DEP_BACKEND STREQUAL "vcpkg")
  find_package(Ogg CONFIG REQUIRED)
  find_package(Vorbis CONFIG REQUIRED)
  find_package(unofficial-theora CONFIG REQUIRED)
  set(THEORAPLAYER_DEP_LIBS
    Ogg::ogg
    Vorbis::vorbis
    Vorbis::vorbisfile
    Vorbis::vorbisenc
    unofficial::theora::theora
    unofficial::theora::theoradec
    unofficial::theora::theoraenc
  )
else ()
  message(FATAL_ERROR "Unsupported THEORAPLAYER_DEP_BACKEND='${THEORAPLAYER_DEP_BACKEND}'.")
endif ()

add_library(theoraplayer SHARED
  ${THEORAPLAYER_ROOT}/theoraplayer/src/AudioInterface.cpp
  ${THEORAPLAYER_ROOT}/theoraplayer/src/AudioInterfaceFactory.cpp
  ${THEORAPLAYER_ROOT}/theoraplayer/src/AudioPacketQueue.cpp
  ${THEORAPLAYER_ROOT}/theoraplayer/src/DataSource.cpp
  ${THEORAPLAYER_ROOT}/theoraplayer/src/Exception.cpp
  ${THEORAPLAYER_ROOT}/theoraplayer/src/FileDataSource.cpp
  ${THEORAPLAYER_ROOT}/theoraplayer/src/FrameQueue.cpp
  ${THEORAPLAYER_ROOT}/theoraplayer/src/Manager.cpp
  ${THEORAPLAYER_ROOT}/theoraplayer/src/MemoryDataSource.cpp
  ${THEORAPLAYER_ROOT}/theoraplayer/src/Mutex.cpp
  ${THEORAPLAYER_ROOT}/theoraplayer/src/theoraplayer.cpp
  ${THEORAPLAYER_ROOT}/theoraplayer/src/Thread.cpp
  ${THEORAPLAYER_ROOT}/theoraplayer/src/Timer.cpp
  ${THEORAPLAYER_ROOT}/theoraplayer/src/Utility.cpp
  ${THEORAPLAYER_ROOT}/theoraplayer/src/VideoClip.cpp
  ${THEORAPLAYER_ROOT}/theoraplayer/src/VideoFrame.cpp
  ${THEORAPLAYER_ROOT}/theoraplayer/src/WorkerThread.cpp
  ${THEORAPLAYER_ROOT}/theoraplayer/src/formats/Theora/VideoClip_Theora.cpp
  ${THEORAPLAYER_ROOT}/theoraplayer/src/YUV/yuv_util.c
  ${THEORAPLAYER_ROOT}/theoraplayer/src/YUV/C/yuv420_grey_c.c
  ${THEORAPLAYER_ROOT}/theoraplayer/src/YUV/C/yuv420_rgb_c.c
  ${THEORAPLAYER_ROOT}/theoraplayer/src/YUV/C/yuv420_yuv_c.c
)

target_include_directories(theoraplayer PRIVATE
  ${THEORAPLAYER_ROOT}/theoraplayer/include
  ${THEORAPLAYER_ROOT}/theoraplayer/include/theoraplayer
  ${THEORAPLAYER_ROOT}/theoraplayer/src
  ${THEORAPLAYER_ROOT}/theoraplayer/src/formats
  ${THEORAPLAYER_ROOT}/theoraplayer/src/YUV
)

set(THEORAPLAYER_PLATFORM_DEFINES)
if (APPLE)
  list(APPEND THEORAPLAYER_PLATFORM_DEFINES _MACOSX)
elseif (UNIX)
  list(APPEND THEORAPLAYER_PLATFORM_DEFINES _LINUX)
endif ()

target_compile_definitions(theoraplayer PRIVATE
  ${THEORAPLAYER_PLATFORM_DEFINES}
  _USE_THEORA
  _YUV_C
  THEORAPLAYER_EXPORTS
  _CRT_SECURE_NO_WARNINGS
)

target_link_libraries(theoraplayer PRIVATE ${THEORAPLAYER_DEP_LIBS})

set_target_properties(theoraplayer PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
  OUTPUT_NAME theoraplayer
)
