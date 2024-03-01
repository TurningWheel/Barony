
find_path(PLAYFAB_INCLUDE_DIR playfab/PlayFabAdminApi.h
  HINTS
    ENV PLAYFAB_DIR
  PATH_SUFFIXES include/playfab include
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /sw # Fink
  /opt/local # DarwinPorts
  /opt/csw # Blastwave
  /opt
)

find_library(PLAYFAB_LIBRARY
  NAMES playfab
  HINTS
    ENV PLAYFAB_DIR
  PATH_SUFFIXES lib lib64
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /sw
  /opt/local
  /opt/csw
  /opt
)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PLAYFAB DEFAULT_MSG PLAYFAB_LIBRARY PLAYFAB_INCLUDE_DIR)
