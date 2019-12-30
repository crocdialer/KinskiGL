INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
SET(LIB_NAME "app")

FILE(GLOB FOLDER_SOURCES App.cpp RemoteControl.cpp ViewerApp.cpp LightComponent.cpp
     Object3DComponent.cpp MaterialComponent.cpp WarpComponent.cpp)
FILE(GLOB FOLDER_HEADERS App.hpp RemoteControl.hpp ViewerApp.hpp LightComponent.hpp
     Object3DComponent.hpp MaterialComponent.hpp WarpComponent.hpp)

##### IMGUI
FILE(GLOB IMGUI_HEADERS imgui/*.h)
FILE(GLOB IMGUI_SOURCES imgui/*.cpp)

##### GLFW
if(NOT KINSKI_ARM)
find_package(glfw3 3.3 REQUIRED)
set(LIBS ${LIBS} glfw)
FILE(GLOB GLFW_SOURCES GLFW_App.cpp App.cpp AntTweakBarConnector.cpp)
FILE(GLOB GLFW_HEADERS GLFW_App.hpp AntTweakBarConnector.hpp)

set(FOLDER_SOURCES ${FOLDER_SOURCES} ${GLFW_SOURCES} ${IMGUI_SOURCES})
set(FOLDER_HEADERS ${FOLDER_HEADERS} ${GLFW_HEADERS} ${IMGUI_HEADERS})

##### RASPI
elseif(KINSKI_RASPI)
FILE(GLOB ARM_SOURCES EGL_App.cpp esUtil.c)
FILE(GLOB ARM_HEADERS EGL_App.hpp esUtil.h)

set(FOLDER_SOURCES ${FOLDER_SOURCES} ${ARM_SOURCES} ${IMGUI_SOURCES})
set(FOLDER_HEADERS ${FOLDER_HEADERS} ${ARM_HEADERS} ${IMGUI_HEADERS})

include_directories("/opt/vc/include/" "/opt/vc/include/interface/vcos/pthreads"
        "/opt/vc/include/interface/vmcs_host/linux" )
link_directories("/opt/vc/lib")
set(LIBS ${LIBS} bcm_host)

##### MALI
elseif(KINSKI_MALI)
FILE(GLOB ARM_SOURCES EGL_App.cpp es_util_mali.c)
FILE(GLOB ARM_HEADERS EGL_App.hpp esUtil.h)
set(FOLDER_SOURCES ${FOLDER_SOURCES} ${ARM_SOURCES})
set(FOLDER_HEADERS ${FOLDER_HEADERS} ${ARM_HEADERS})
include_directories("/usr/include/libdrm/")
endif()

add_library(${LIB_NAME} ${LIB_TYPE} ${FOLDER_SOURCES} ${FOLDER_HEADERS})
TARGET_LINK_LIBRARIES(${LIB_NAME} ${LIBS})

SET(LIBS ${LIBS} ${LIB_NAME})

SOURCE_GROUP("Headers" FILES ${FOLDER_HEADERS})
SOURCE_GROUP("Sources" FILES ${FOLDER_SOURCES})

install (TARGETS ${LIB_NAME} DESTINATION lib)
install (FILES ${FOLDER_HEADERS} DESTINATION "include/${LIB_NAME}")
