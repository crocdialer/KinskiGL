# Leap Motion
find_library(LEAP_LIBRARIES NAMES leap
             HINTS ${CMAKE_SOURCE_DIR}/blocks/leap/lib)
set(MODULE_INCLUDES "${CMAKE_SOURCE_DIR}/blocks/leap/include" PARENT_SCOPE)
set(MODULE_LIBRARIES ${LEAP_LIBRARIES} PARENT_SCOPE)
