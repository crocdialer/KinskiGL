##### BULLET + OPENCV
find_package(BULLET REQUIRED)
INCLUDE_DIRECTORIES(${BULLET_INCLUDE_DIRS})
set(LIBS ${LIBS} ${BULLET_LIBRARIES})

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})
set(LIBS ${LIBS} ${OpenCV_LIBS})
#####

set(LIBS ${LIBS} "kinskiCV")

addSample(${itemName} ${folderItem})
