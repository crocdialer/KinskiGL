##### Bluetooth
if(APPLE)
  find_library(CoreBluetooth NAMES CoreBluetooth)
  set(MODULE_LIBRARIES ${CoreBluetooth} PARENT_SCOPE)
  FILE(GLOB MODULE_SOURCES *.c* osx/*.mm)
elseif(UNIX)
  find_package(PkgConfig REQUIRED)
  pkg_search_module(GLIB REQUIRED glib-2.0)
  find_library(bluetooth NAMES bluetooth)
  set(MODULE_INCLUDE_DIRS ${GLIB_INCLUDE_DIRS})
  set(MODULE_LIBRARIES ${bluetooth} ${GLIB_LIBRARIES} PARENT_SCOPE)
  FILE(GLOB MODULE_SOURCES *.c* linux/*.c* linux/bluez/*.c)
endif()

# common header files
FILE(GLOB MODULE_HEADERS bluetooth.hpp UUID.hpp Bluetooth_UART.hpp)

# forward variables to parent scope
set(MODULE_INCLUDE_DIRS ${MODULE_INCLUDE_DIRS} PARENT_SCOPE)
set(MODULE_HEADERS ${MODULE_HEADERS} PARENT_SCOPE)
set(MODULE_SOURCES ${MODULE_SOURCES} PARENT_SCOPE)
