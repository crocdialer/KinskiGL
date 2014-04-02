if(APPLE)
  find_library(CoreFoundation NAMES CoreFoundation)
  find_library(CoreVideo NAMES CoreVideo)
  find_library(QuickTime NAMES QuickTime)
  find_library(QTKit NAMES QTKit)
  set(MODULE_LIBRARIES ${CoreFoundation} ${CoreVideo} ${QuickTime} ${QTKit} PARENT_SCOPE)
endif()

set(MODULE_INCLUDES "" PARENT_SCOPE)
