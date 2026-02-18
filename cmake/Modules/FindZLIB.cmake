# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#.rst:
# FindZLIB
# ----------
#
#
#
# Locate ZLIB library This module defines ZLIB_LIBRARY, the name of
# the library to link against ZLIB_FOUND, if false, do not try to link
# to ZLIB ZLIB_INCLUDE_DIR, where to find zlib.h
#
# $ZLIB_DIR is an environment variable that would correspond to the
# ./configure --prefix=$ZLIB_DIR used in building ZLIB.
#
# Created by Eric Wing.

message ("USING OVERRIDE FINDZLIB!")

find_path(ZLIB_INCLUDE_DIR zlib.h
  HINTS
    ENV ZLIB_DIR
	${ZLIB_DIR}
  PATH_SUFFIXES include
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local
  /usr
  /sw # Fink
  /opt/local # DarwinPorts
  /opt/csw # Blastwave
  /opt
)

find_library(ZLIB_LIBRARY
  NAMES zlib libz z
  HINTS
    ENV ZLIB_DIR
	${ZLIB_DIR}
  PATH_SUFFIXES lib
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local
  /usr
  /sw
  /opt/local
  /opt/csw
  /opt
)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(ZLIB DEFAULT_MSG ZLIB_LIBRARY ZLIB_INCLUDE_DIR)
