set(LIBS ${LIBS})
set(MODULES rtaudio)
add_definitions(-D__MACOSX_CORE__)
KINSKI_ADD_SAMPLE(${itemName} ${folderItem} "${MODULES}")

