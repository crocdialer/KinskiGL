if(APPLE)
  find_library(CoreAudio NAMES CoreAudio)
  add_definitions(-D__MACOSX_CORE__ PARENT_SCOPE)
  set(MODULE_LIBRARIES ${CoreAudio} PARENT_SCOPE)
elseif(UNIX)
  #TODO
  add_definitions(-D__LINUX_ALSA__)
  find_library(asound NAMES ASOUND)
  set(MODULE_LIBRARIES ${ASOUND} PARENT_SCOPE)
endif()

set(MODULE_INCLUDES "" PARENT_SCOPE)
