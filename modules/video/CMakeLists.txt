if(APPLE)
  find_library(AVFoundation NAMES AVFoundation)
  find_library(CoreMedia NAMES CoreMedia)
  find_library(CoreFoundation NAMES CoreFoundation)
  find_library(IOSurface NAMES IOSurface)
  set(MODULE_LIBRARIES ${AVFoundation} ${CoreFoundation} ${CoreMedia} ${IOSurface} PARENT_SCOPE)
  
  # OSX specific implementation
  FILE(GLOB MODULE_SOURCES MovieController.mm CameraController.mm PARENT_SCOPE)
else if(KINSKI_RASPI)
  # Raspian specific implementation
  FILE(GLOB MODULE_SOURCES MovieController_rpi.cpp PARENT_SCOPE)
endif()

set(MODULE_INCLUDES "" PARENT_SCOPE)

# common header files
FILE(GLOB MODULE_HEADERS MovieController.h CameraController.h PARENT_SCOPE)
