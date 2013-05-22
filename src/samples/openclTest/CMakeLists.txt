##### OPENCL
find_package(OpenCL REQUIRED)
INCLUDE_DIRECTORIES(${OpenCL_INCLUDE_DIRS})
set(LIBS ${LIBS} ${OPENCL_LIBRARIES})
#####

addSample(${itemName} ${folderItem})
