# Locate the glfw library
# This module defines the following variables:
# GLFW_LIBRARY, the name of the library;
# GLFW_INCLUDE_DIR, where to find glfw include files.
# GLFW_FOUND, true if both the GLFW_LIBRARY and GLFW_INCLUDE_DIR have been found.
#
# To help locate the library and include file, you could define an environment variable called
# GLFW_ROOT which points to the root of the glfw library installation. This is pretty useful
# on a Windows platform.
#
#
# Usage example to compile an "executable" target to the glfw library:
#
# FIND_PACKAGE (glfw REQUIRED)
# INCLUDE_DIRECTORIES (${GLFW_INCLUDE_DIR})
# ADD_EXECUTABLE (executable ${EXECUTABLE_SRCS})
# TARGET_LINK_LIBRARIES (executable ${GLFW_LIBRARY})
#
# TODO:
# Allow the user to select to link to a shared library or to a static library.

#Search for the include file...
FIND_PATH(GLFW_INCLUDE_DIRS GL/glfw.h DOC "Path to GLFW include directory."
  HINTS
  $ENV{GLFW_ROOT}
  PATH_SUFFIX include #For finding the include file under the root of the glfw expanded archive, typically on Windows.
  PATHS
  /usr/include/
  /usr/local/include/
  # By default headers are under GL subfolder
  /usr/include/GL
  /usr/local/include/GL
  ${GLFW_ROOT_DIR}/include/ # added by ptr
 
)

if(NOT GLFW_USE_STATIC_LIBS)
set(GLFW_USE_STATIC_LIBS ON)
endif()

if (GLFW_USE_STATIC_LIBS)
    SET(GLFW_LIB_NAMES libglfw.a glfw GLFW.lib)
else(GLFW_USE_STATIC_LIBS)
    SET(GLFW_LIB_NAMES glfw GLFW.lib)
endif(GLFW_USE_STATIC_LIBS)

FIND_LIBRARY(GLFW_LIBRARIES DOC "Absolute path to GLFW library."
  NAMES ${GLFW_LIB_NAMES}
  HINTS
  $ENV{GLFW_ROOT}
  PATH_SUFFIXES lib/win32 #For finding the library file under the root of the glfw expanded archive, typically on Windows.
  PATHS
  /usr/local/lib
  /usr/lib
  ${GLFW_ROOT_DIR}/lib-msvc100/release # added by ptr
)
IF( APPLE AND GLFW_USE_STATIC_LIBS)
    find_library(IOKIT NAMES IOKit)
    find_library(COCOA NAMES Cocoa)
    SET(GLFW_LIBRARIES ${GLFW_LIBRARIES} ${IOKIT} ${COCOA})
endif( APPLE AND GLFW_USE_STATIC_LIBS )
#FIND_LIBRARY(GLFW_LIBRARIES "/usr/local/lib/libglfw.a")
#SET(GLFW_LIBRARIES "/usr/local/lib/libglfw.a")

SET(GLFW_FOUND 0)
IF(GLFW_LIBRARIES AND GLFW_INCLUDE_DIRS)
  SET(GLFW_FOUND 1)
  message(STATUS "Found GLFW " ${GLFW_LIBRARIES})
ELSE()
  message(STATUS "GLFW NOT found!")
ENDIF(GLFW_LIBRARIES AND GLFW_INCLUDE_DIRS)
