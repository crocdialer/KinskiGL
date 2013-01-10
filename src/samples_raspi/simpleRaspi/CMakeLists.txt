find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})
set(LIBS ${LIBS} ${OpenCV_LIBS})
#####

#link_directories("/opt/vc/lib/")
#set(LIBS ${LIBS} "EGL" "GLESv2")
#message(${LIBS})

addSample(${itemName} ${folderItem})
