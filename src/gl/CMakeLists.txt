# which shaders to include
if(KINSKI_ARM)
  set(SHADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders/ES2")
else()
  set(SHADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders/410")
endif()

#stringify all shader files
STRINGIFY_SHADERS(${SHADER_DIR})

# now gather all files
FILE(GLOB FOLDER_SOURCES *.c*)
FILE(GLOB FOLDER_HEADERS *.h*)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
SET(LIB_NAME "gl")

add_library(${LIB_NAME} ${LIB_TYPE} ${FOLDER_SOURCES} ${FOLDER_HEADERS})

OPTION (REPORT_GL_ERRORS "Compile in checks for GL_ERRORs. Only for debugging, expect performance hit" OFF)

if(REPORT_GL_ERRORS)
   add_definitions(-DKINSKI_GL_REPORT_ERRORS)
endif(REPORT_GL_ERRORS)

if(BUILD_IOS)
set(LIBS ${LIBS} "-framework OpenGLES")
elseif(KINSKI_RASPI)
include_directories("/opt/vc/include/" "/opt/vc/include/interface/vcos/pthreads")
link_directories("/opt/vc/lib")
set(LIBS ${LIBS} "EGL" "GLESv2")
SET_TARGET_PROPERTIES(${LIB_NAME} PROPERTIES LINK_FLAGS -Wl,-L/opt/vc/lib)
elseif(KINSKI_MALI)
find_package(PkgConfig REQUIRED)
pkg_search_module(LIB_DRM REQUIRED libdrm)
include_directories(${LIB_DRM_INCLUDE_DIRS})
link_directories(${LIB_DRM_LIBRARY_DIRS})
SET_TARGET_PROPERTIES(${LIB_NAME} PROPERTIES LINK_FLAGS -Wl,-L/usr/lib/arm-linux-gnueabihf/)
set(LIBS ${LIBS} "${LIB_DRM_LIBRARIES}" "EGL" "GLESv2" "gbm")
else()
##### OPENGL
find_package(OpenGL REQUIRED)
INCLUDE_DIRECTORIES(${OpenGL_INCLUDE_DIRS})
set(LIBS ${LIBS} ${OPENGL_LIBRARIES})
endif()

TARGET_LINK_LIBRARIES(${LIB_NAME} ${LIBS})

SET(LIBS ${LIBS} ${LIB_NAME})

SOURCE_GROUP("Headers" FILES ${FOLDER_HEADERS})
SOURCE_GROUP("Sources" FILES ${FOLDER_SOURCES})

install (TARGETS ${LIB_NAME} DESTINATION lib)
install (FILES ${FOLDER_HEADERS} DESTINATION "include/${LIB_NAME}")

addTestMacro()
