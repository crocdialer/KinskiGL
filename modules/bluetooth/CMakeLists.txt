##### Bluetooth
if(APPLE)
  find_library(CoreBluetooth NAMES CoreBluetooth)
  set(MODULE_LIBRARIES ${CoreBluetooth} PARENT_SCOPE)
elseif(UNIX)
endif()

set(MODULE_INCLUDES "" PARENT_SCOPE)
