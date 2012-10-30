##### ASSIMP
find_package(ASSIMP REQUIRED)
INCLUDE_DIRECTORIES(${ASSIMP_INCLUDE_DIRS})
set(LIBS ${LIBS} ${ASSIMP_LIBRARIES})

addSample(${itemName} ${folderItem})
