# - Locate FMOD library (By Matt Raykowski, OpenNeL Project http://www.opennel.org/)
# http://www.opennel.org/fisheye/browse/~raw,r=1353/NeL/trunk/nel/CMakeModules/FindFMOD.cmake
# (with permission to relicense as LGPL-with-staticlink-exemption by Matt Raykowski)
# This module defines
#  FMOD_LIBRARY, the library to link against
#  FMOD_FOUND, if false, do not try to link to FMOD
#  FMOD_INCLUDE_DIR, where to find headers.

IF(FMOD_LIBRARY AND FMOD_INCLUDE_DIR)
  # in cache already
  SET(FMOD_FIND_QUIETLY TRUE)
ENDIF(FMOD_LIBRARY AND FMOD_INCLUDE_DIR)


FIND_PATH(FMOD_INCLUDE_DIR
  fmod.h
  PATHS
  $ENV{FMOD_DIR}/include
  /usr/local/include
  /usr/local/include/fmodex/
  /usr/include
  /usr/include/fmodex/
  /sw/include
  /sw/include/fmodex/
  /opt/local/include
  /opt/local/include/fmodex/
  /opt/csw/include
  /opt/csw/include/fmodex/
  /opt/include
  /opt/include/fmodex/
  PATH_SUFFIXES fmod fmod3
)

FIND_LIBRARY(FMOD_LIBRARY
  NAMES fmod fmodex fmodvc libfmod libfmodex
  PATHS
  $ENV{FMOD_DIR}/lib
  /usr/local/lib
  /usr/lib
  /usr/local/X11R6/lib
  /usr/X11R6/lib
  /sw/lib
  /opt/local/lib
  /opt/csw/lib
  /opt/lib
  /usr/freeware/lib64
)

IF(FMOD_LIBRARY AND FMOD_INCLUDE_DIR)
  SET(FMOD_FOUND "YES")
  SET( FMOD_LIBRARIES ${FMOD_LIBRARY} )
  IF(NOT FMOD_FIND_QUIETLY)
    MESSAGE(STATUS "Found FMOD: ${FMOD_LIBRARY}")
  ENDIF(NOT FMOD_FIND_QUIETLY)
ELSE(FMOD_LIBRARY AND FMOD_INCLUDE_DIR)
  IF(NOT FMOD_FIND_QUIETLY)
    MESSAGE(STATUS "Warning: Unable to find FMOD!")
  ENDIF(NOT FMOD_FIND_QUIETLY)
ENDIF(FMOD_LIBRARY AND FMOD_INCLUDE_DIR)
