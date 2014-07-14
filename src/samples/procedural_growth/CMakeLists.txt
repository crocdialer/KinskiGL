set(LIBS ${LIBS})
set(MODULES avfoundation rtaudio fmod assimp)
add_definitions(-D__MACOSX_CORE__)
KINSKI_ADD_SAMPLE(${itemName} ${folderItem} "${MODULES}")