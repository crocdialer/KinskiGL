# Leap Motion
if(USE_CXX11)
  find_library(LEAP_LIBRARIES NAMES leap
              HINTS ${CMAKE_SOURCE_DIR}/blocks/leap/libc++)
else()
  find_library(LEAP_LIBRARIES NAMES leap
              HINTS ${CMAKE_SOURCE_DIR}/blocks/leap/lib)
endif(USE_CXX11)

set(MODULE_INCLUDES "${CMAKE_SOURCE_DIR}/blocks/leap/include" PARENT_SCOPE)
set(MODULE_LIBRARIES ${LEAP_LIBRARIES} PARENT_SCOPE)
