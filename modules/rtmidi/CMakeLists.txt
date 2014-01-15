if(APPLE)
find_library(CoreMIDI NAMES CoreMIDI)
find_library(CoreFoundation NAMES CoreFoundation)
find_library(CoreAudio NAMES CoreAudio)
add_definitions(-D__MACOSX_CORE__)
set(MODULE_LIBRARIES ${CoreMIDI} ${CoreFoundation} ${CoreAudio} PARENT_SCOPE)
elseif(UNIX)
  #TODO
endif()

set(MODULE_INCLUDES "" PARENT_SCOPE)
