# - Find nativefiledialog includes and library
#
# This module defines
#  NFD_INCLUDE_DIRS, the library's header files
#  NFD_LIBRARIES, the libraries to link against to use NFD.
#  NFD_FOUND, If false, do not try to use NFD.
# also defined, but not for general use are
#  NFD_LIBRARY, where to find the PNG library.

# Searches the environment variable NFD_DIR

# This library is organized sloppily as is.
# What I'd do is the following:
# After you build the bloody thing, create a release-dir
# copy libnfd.a to release-dir/lib/libnfd.a
# copy the header file from src/include/nfd.h to release-dir/include/nfd/nfd.h
# set an environment variable: NFD_SEARCH_PATHS to wherever release-dir/ is.

SET(NFD_SEARCH_PATHS
	${NFD_SEARCH_PATHS}
	$ENV{NFD_DIR}
	~/Library/Frameworks
	/Library/Frameworks
	/usr/local
	/usr
	/sw # Fink
	/opt/local # DarwinPorts
	/opt/csw # Blastwave
	/opt
)


FIND_PATH(NFD_INCLUDE_DIR nfd.h
	HINTS
	$ENV{NFD_DIR}
	PATH_SUFFIXES include/nfd include nfd
	PATHS ${NFD_SEARCH_PATHS}
)

FIND_LIBRARY(NFD_LIBRARY
	NAMES nfd libnfd
	HINTS
	$ENV{NFD_DIR}
	PATH_SUFFIXES lib64 lib libs64 libs libs/Win32 libs/Win64 nfd
	PATHS ${NFD_SEARCH_PATHS}
)

#SET(NFD_NAMES ${NFD_NAMES} nfd libnfd)
#FIND_LIBRARY(NFD_LIBRARY NAMES ${NFD_NAMES} )

set(NFD_LIBRARIES ${NFD_LIBRARY})
set(NFD_INCLUDE_DIRS ${NFD_INCLUDE_DIR})

# handle the QUIETLY and REQUIRED arguments and set PNG_FOUND to TRUE if
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(NFD
										REQUIRED_VARS NFD_LIBRARIES NFD_INCLUDE_DIRS)

MARK_AS_ADVANCED(NFD_INCLUDE_DIR NFD_LIBRARY)
