if(APPLE)
  find_library(AVFoundation NAMES AVFoundation)
  find_library(CoreMedia NAMES CoreMedia)
  find_library(CoreFoundation NAMES CoreFoundation)
  find_library(IOSurface NAMES IOSurface)
  set(MODULE_LIBRARIES ${AVFoundation} ${CoreFoundation} ${CoreMedia} ${IOSurface} PARENT_SCOPE)
  
  # OSX specific implementation
  FILE(GLOB MODULE_SOURCES MovieController.mm CameraController.mm)
elseif(KINSKI_RASPI)

  # Raspian specific implementation
  FILE(GLOB MODULE_SOURCES MovieController_rpi.cpp ilclient.h ilclient.c ilcore.c)
endif()

set(MODULE_INCLUDES "" PARENT_SCOPE)

# common header files
FILE(GLOB MODULE_HEADERS MovieController.h CameraController.h)

# forward variables to parent scope
set(MODULE_HEADERS ${MODULE_HEADERS} PARENT_SCOPE)
set(MODULE_SOURCES ${MODULE_SOURCES} PARENT_SCOPE)
