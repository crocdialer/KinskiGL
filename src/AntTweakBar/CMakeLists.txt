#FILE(GLOB FOLDER_SOURCES *.c*)
#FILE(GLOB FOLDER_HEADERS *.h)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
SET(LIB_NAME "AntTweakBar")

SET(OUR_FLAGS "-Wall -fPIC -fno-strict-aliasing -D__PLACEMENT_NEW_INLINE")

if(APPLE)
    SET(FOLDER_SOURCES TwColors.cpp TwFonts.cpp TwOpenGL.cpp TwOpenGLCore.cpp TwBar.cpp TwMgr.cpp TwPrecomp.cpp LoadOGL.cpp LoadOGLCore.cpp TwEventGLFW.c TwEventGLUT.c TwEventSDL.c TwEventSDL12.c TwEventSDL13.c TwEventSFML.cpp)
    SET(FOLDER_HEADERS AntPerfTimer.h LoadOGLCore.h MiniGLUT.h MiniSDL13.h TwBar.h TwGraph.h TwOpenGL.h TwPrecomp.h resource.h LoadOGL.h MiniGLFW.h MiniSDL12.h MiniSFML16.h TwColors.h TwFonts.h      TwMgr.h TwOpenGLCore.h)

    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OUR_FLAGS} -x objective-c++")
    ADD_DEFINITIONS("-D_MACOSX") 
endif(APPLE)

add_library(${LIB_NAME} STATIC ${FOLDER_SOURCES} ${FOLDER_HEADERS})

#set(LIBS "-framework OpenGL -framework GLUT -framework AppKit")
TARGET_LINK_LIBRARIES(${LIB_NAME} "-framework OpenGL -framework GLUT -framework AppKit")

SET(LIBS ${LIBS} ${LIB_NAME})

SOURCE_GROUP("Headers" FILES ${FOLDER_HEADERS})
SOURCE_GROUP("Sources" FILES ${FOLDER_SOURCES})

install (TARGETS ${LIB_NAME} DESTINATION lib)    

