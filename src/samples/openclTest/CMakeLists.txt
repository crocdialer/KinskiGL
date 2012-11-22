##### OPENCL
find_package(OpenCL REQUIRED)
INCLUDE_DIRECTORIES(${OpenCL_INCLUDE_DIRS})
set(LIBS ${LIBS} ${OPENCL_LIBRARIES})

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})
set(LIBS ${LIBS} ${OpenCV_LIBS})
#####

addSample(${itemName} ${folderItem})
