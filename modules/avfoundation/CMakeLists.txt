if(APPLE)
  find_library(AVFoundation NAMES AVFoundation)
  find_library(CoreMedia NAMES CoreMedia)
  find_library(CoreFoundation NAMES CoreFoundation)
  find_library(IOSurface NAMES IOSurface)
  set(MODULE_LIBRARIES ${AVFoundation} ${CoreFoundation} ${CoreMedia} ${IOSurface} PARENT_SCOPE)
endif()

set(MODULE_INCLUDES "" PARENT_SCOPE)
