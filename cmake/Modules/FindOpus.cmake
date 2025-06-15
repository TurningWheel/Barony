
find_path(OPUS_INCLUDE_DIR opus/opus.h
  HINTS
    ENV OPUS_DIR
  PATH_SUFFIXES include/opus include
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /sw # Fink
  /opt/local # DarwinPorts
  /opt/csw # Blastwave
  /opt
)

find_library(OPUS_LIBRARY
  NAMES opus
  HINTS
    ENV OPUS_DIR
  PATH_SUFFIXES lib lib64
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /sw
  /opt/local
  /opt/csw
  /opt
)

set(OPUS_LIBRARIES ${OPUS_LIBRARY})

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Opus DEFAULT_MSG OPUS_LIBRARIES OPUS_INCLUDE_DIR)
