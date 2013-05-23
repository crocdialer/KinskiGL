FILE(GLOB FOLDER_SOURCES *.c*)
FILE(GLOB FOLDER_HEADERS *.h)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
SET(LIB_NAME "kinskiGL")

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
else()
##### OPENGL
find_package(OpenGL REQUIRED)
INCLUDE_DIRECTORIES(${OpenGL_INCLUDE_DIRS})
set(LIBS ${LIBS} ${OPENGL_LIBRARIES})
endif()

add_library(${LIB_NAME} ${LIB_TYPE} ${FOLDER_SOURCES} ${FOLDER_HEADERS})
TARGET_LINK_LIBRARIES(${LIB_NAME} ${LIBS})

SET(LIBS ${LIBS} ${LIB_NAME})

SOURCE_GROUP("Headers" FILES ${FOLDER_HEADERS})
SOURCE_GROUP("Sources" FILES ${FOLDER_SOURCES})

install (TARGETS ${LIB_NAME} DESTINATION lib)    
install (FILES ${FOLDER_HEADERS} DESTINATION "include/${LIB_NAME}")    

addTestMacro()
