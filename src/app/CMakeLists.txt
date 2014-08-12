FILE(GLOB FOLDER_SOURCES *.c*)
FILE(GLOB FOLDER_HEADERS *.h)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
SET(LIB_NAME "app")

##### GLFW
if(NOT KINSKI_RASPI)
find_package(GLFW3 REQUIRED)
INCLUDE_DIRECTORIES(${GLFW_INCLUDE_DIRS})
SET(LIBS ${LIBS} ${GLFW_LIBRARIES})
FILE(GLOB FOLDER_SOURCES GLFW_App.cpp App.cpp AppServer.cpp ViewerApp.cpp LightComponent.cpp Object3DComponent.cpp 
        AntTweakBarConnector.cpp OutstreamGL.cpp MaterialComponent.cpp)
FILE(GLOB FOLDER_HEADERS GLFW_App.h App.h AppServer.h ViewerApp.h LightComponent.h Object3DComponent.h 
        AntTweakBarConnector.h OutstreamGL.h MaterialComponent.h)

##### RASPI
else()
FILE(GLOB FOLDER_SOURCES Raspi_App.cpp AppServer.cpp esUtil.c App.cpp TextureIO.cpp OutstreamGL.cpp)
FILE(GLOB FOLDER_HEADERS Raspi_App.h AppServer.h esUtil.h App.h TextureIO.h OutstreamGL.h)
include_directories("/opt/vc/include/" "/opt/vc/include/interface/vcos/pthreads"
        "/opt/vc/include/interface/vmcs_host/linux" )
link_directories("/opt/vc/lib")
endif()

add_library(${LIB_NAME} ${LIB_TYPE} ${FOLDER_SOURCES} ${FOLDER_HEADERS})
TARGET_LINK_LIBRARIES(${LIB_NAME} ${LIBS})

SET(LIBS ${LIBS} ${LIB_NAME})

SOURCE_GROUP("Headers" FILES ${FOLDER_HEADERS})
SOURCE_GROUP("Sources" FILES ${FOLDER_SOURCES})

install (TARGETS ${LIB_NAME} DESTINATION lib)    
install (FILES ${FOLDER_HEADERS} DESTINATION "include/${LIB_NAME}")