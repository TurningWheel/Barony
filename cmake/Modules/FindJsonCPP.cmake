
find_path(JSONCPP_INCLUDE_DIR json/json.h
  HINTS
    ENV JSONCPP_DIR
  PATH_SUFFIXES include/json include
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /sw # Fink
  /opt/local # DarwinPorts
  /opt/csw # Blastwave
  /opt
)

find_library(JSONCPP_LIBRARY
  NAMES jsoncpp
  HINTS
    ENV JSONCPP_DIR
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
FIND_PACKAGE_HANDLE_STANDARD_ARGS(JsonCPP DEFAULT_MSG JSONCPP_LIBRARY JSONCPP_INCLUDE_DIR)
