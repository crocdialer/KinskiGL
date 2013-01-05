FILE(GLOB FOLDER_SOURCES *.c*)
FILE(GLOB FOLDER_HEADERS *.h)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
SET(LIB_NAME "kinskiApp")

##### GLFW
if(NOT KINSKI_RASPI)
find_package(GLFW REQUIRED)
INCLUDE_DIRECTORIES(${GLFW_INCLUDE_DIRS})
SET(LIBS ${LIBS} ${GLFW_LIBRARIES})
FILE(GLOB FOLDER_SOURCES GLFW_App.cpp App.cpp TextureIO.cpp AntTweakBarConnector.cpp)
FILE(GLOB FOLDER_HEADERS GLFW_App.h App.h TextureIO.h AntTweakBarConnector.h)
#####
else()
FILE(GLOB FOLDER_SOURCES Raspi_App.cpp esUtil.c App.cpp)
FILE(GLOB FOLDER_HEADERS Raspi_App.h esUtil.h App.h)

include_directories("/opt/vc/include/" "/opt/vc/include/interface/vcos/pthreads")
link_directories("/opt/vc/lib")
endif()

add_library(${LIB_NAME} ${LIB_TYPE} ${FOLDER_SOURCES} ${FOLDER_HEADERS})
TARGET_LINK_LIBRARIES(${LIB_NAME} ${LIBS})

SET(LIBS ${LIBS} ${LIB_NAME})

SOURCE_GROUP("Headers" FILES ${FOLDER_HEADERS})
SOURCE_GROUP("Sources" FILES ${FOLDER_SOURCES})

install (TARGETS ${LIB_NAME} DESTINATION lib)    
install (FILES ${FOLDER_HEADERS} DESTINATION "include/${LIB_NAME}")    
