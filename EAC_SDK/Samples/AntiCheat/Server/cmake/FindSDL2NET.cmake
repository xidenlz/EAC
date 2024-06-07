FIND_PATH(SDL2NET_INCLUDE_DIR SDL_net.h
  HINTS
  $ENV{SDL2NETDIR}
  PATH_SUFFIXES include/SDL2 include
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local/include/SDL2
  /usr/include/SDL2
  /sw 
  /opt/local
  /opt/csw
  /opt
)

FIND_LIBRARY(SDL2NET_LIBRARY
  NAMES SDL2_net
  HINTS
  $ENV{SDL2NETDIR}
  /usr/lib
  /usr/lib64
  PATH_SUFFIXES lib64 lib
  PATHS
  /sw
  /opt/local
  /opt/csw
  /opt
)

INCLUDE(FindPackageHandleStandardArgs)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(SDL2NET REQUIRED_VARS SDL2NET_LIBRARY SDL2NET_INCLUDE_DIR)