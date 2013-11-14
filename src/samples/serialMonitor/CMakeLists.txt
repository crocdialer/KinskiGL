if(APPLE)
find_library(CoreMIDI NAMES CoreMIDI)
find_library(CoreFoundation NAMES CoreFoundation)
find_library(CoreAudio NAMES CoreAudio)
add_definitions(-D__MACOSX_CORE__)
set(LIBS ${LIBS} ${CoreMIDI} ${CoreFoundation} ${CoreAudio})
endif()

addSample(${itemName} ${folderItem})

