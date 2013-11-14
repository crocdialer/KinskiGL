if(APPLE)
find_library(CoreMIDI NAMES CoreMIDI)
find_library(CoreFoundation NAMES CoreFoundation)
find_library(CoreAudio NAMES CoreAudio)
set(LIBS ${LIBS} ${CoreMIDI} ${CoreFoundation} ${CoreAudio})
endif()

addSample(${itemName} ${folderItem})

