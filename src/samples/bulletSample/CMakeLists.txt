##### OPENCV
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})
set(LIBS ${LIBS} ${OpenCV_LIBS})
#####

set(LIBS ${LIBS} "kinskiCV")

set(MODULES bullet)
KINSKI_ADD_SAMPLE(${itemName} ${folderItem} "${MODULES}")

