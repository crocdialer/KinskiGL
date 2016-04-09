##### Bluetooth
if(APPLE)
  find_library(CoreBluetooth NAMES CoreBluetooth)
  set(MODULE_LIBRARIES ${CoreBluetooth} PARENT_SCOPE)
  FILE(GLOB MODULE_SOURCES *.c* osx/*.mm)
elseif(UNIX)
  find_library(bluetooth NAMES bluetooth)
  set(MODULE_LIBRARIES ${bluetooth} PARENT_SCOPE)
  FILE(GLOB MODULE_SOURCES *.c* linux/*.c*)
endif()

set(MODULE_INCLUDES "" PARENT_SCOPE)

# common header files
FILE(GLOB MODULE_HEADERS MovieController.h CameraController.h)

# forward variables to parent scope
set(MODULE_INCLUDE_DIRS ${MODULE_INCLUDE_DIRS} PARENT_SCOPE)
set(MODULE_HEADERS ${MODULE_HEADERS} PARENT_SCOPE)
set(MODULE_SOURCES ${MODULE_SOURCES} PARENT_SCOPE)
