#Search for the include file...
FIND_PATH(FMOD_INCLUDE_DIRS fmod.h DOC "Path to FMOD include directory."
  HINTS
  $ENV{FMOD_ROOT}
  PATH_SUFFIX include #For finding the include file under the root of the glfw expanded archive, typically on Windows.
  PATHS
  ${CMAKE_SOURCE_DIR}/libs/fmod/include
  /usr/include/
  /usr/local/include/
  /usr/include/fmod
  /usr/local/include/fmod
  ${FMOD_ROOT}/include/ 
)

SET(FMOD_LIB_NAMES fmodex)

IF( APPLE )
    set(LIB_SEARCH_PATHS "${CMAKE_SOURCE_DIR}/libs/fmod/lib_osx")    
endif( APPLE )

FIND_LIBRARY(FMOD_LIBRARIES DOC "Absolute path to FMOD library."
  NAMES ${FMOD_LIB_NAMES}
  HINTS
  $ENV{FMOD_ROOT}
  PATH_SUFFIXES lib/win32 
  PATHS
  ${LIB_SEARCH_PATHS}
  /usr/local/lib
  /usr/lib
  ${FMOD_ROOT}/lib-msvc100/release 
)

SET(FMOD_FOUND 0)
IF(FMOD_LIBRARIES AND FMOD_INCLUDE_DIRS)
  SET(FMOD_FOUND 1)
  message(STATUS "Found FMOD -> " ${FMOD_LIBRARIES})
ELSE()
  message(STATUS "FMOD NOT found!")
ENDIF(FMOD_LIBRARIES AND FMOD_INCLUDE_DIRS)
