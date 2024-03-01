
find_path(THEORAPLAYER_INCLUDE_DIR theoraplayer/theoraplayer.h
  HINTS
    ENV THEORAPLAYER_DIR
  PATH_SUFFIXES include/theoraplayer include
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /sw # Fink
  /opt/local # DarwinPorts
  /opt/csw # Blastwave
  /opt
)

find_library(THEORAPLAYER_LIBRARY
  NAMES theoraplayer
  HINTS
    ENV THEORAPLAYER_DIR
  PATH_SUFFIXES lib lib64
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /sw
  /opt/local
  /opt/csw
  /opt
)

find_library(THEORA_LIBRARY
  NAMES theora
  HINTS
    ENV THEORAPLAYER_DIR
  PATH_SUFFIXES lib lib64
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /sw
  /opt/local
  /opt/csw
  /opt
)

set(THEORAPLAYER_LIBRARIES ${THEORAPLAYER_LIBRARY} ${THEORA_LIBRARY})

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(TheoraPlayer DEFAULT_MSG THEORAPLAYER_LIBRARIES THEORAPLAYER_INCLUDE_DIR)
