cmake_minimum_required (VERSION 2.8)

project (fwglTest)

MESSAGE(${CMAKE_SOURCE_DIR})

SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake_scripts
                      ${CMAKE_MODULE_PATH})

##### PKGCONFIG
#find_package(PkgConfig REQUIRED)
#####

##### OPENGL
find_package(OpenGL REQUIRED)
INCLUDE_DIRECTORIES(${OpenGL_INCLUDE_DIRS})
set(LIBS ${LIBS} ${OPENGL_LIBRARIES})

##### BOOST
find_package(Boost REQUIRED COMPONENTS thread)
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
set(LIBS ${LIBS} ${Boost_LIBRARIES})
#####

##### GLFW
#pkg_search_module(GLFW REQUIRED libglfw)
find_package(GLFW REQUIRED)
INCLUDE_DIRECTORIES(${GLFW_INCLUDE_DIRS})
SET(LIBS ${LIBS} ${GLFW_LIBRARIES})
#####

#####
find_package(GLEW REQUIRED)
INCLUDE_DIRECTORIES(${GLEW_INCLUDE_DIRS})
set(LIBS ${LIBS} ${GLEW_LIBRARIES})
##### 

##### OPENCV
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})
set(LIBS ${LIBS} ${OpenCV_LIBS})
#####

IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

### source files
SET(SRC_DIR src)
INCLUDE_DIRECTORIES(${SRC_DIR})

FILE(GLOB project_SOURCES ${SRC_DIR}/*.c*)
FILE(GLOB project_HEADERS ${SRC_DIR}/*.h*)

FILE(GLOB_RECURSE resFiles
  ${CMAKE_SOURCE_DIR}/res/*
)

### define all input sources + headers
SET(ALL_STUFF  ${project_SOURCES} ${project_HEADERS})

# Source Groups affect grouping in IDEs like MSVC or XCode
SOURCE_GROUP("Headers" FILES ${project_HEADERS})
SOURCE_GROUP("Sources" FILES ${project_SOURCES})
SOURCE_GROUP("Project Resources" FILES ${resFiles})

SET(PROGNAME fwglTest)

IF( APPLE )
SET( MACOSX_BUNDLE_ICON_FILE res/kinski.icns )

SET_SOURCE_FILES_PROPERTIES(
  ${resFiles}
  PROPERTIES
  MACOSX_PACKAGE_LOCATION Resources
)

ADD_EXECUTABLE(${PROGNAME} MACOSX_BUNDLE ${ALL_STUFF} ${resFiles})

ELSE( APPLE )
ADD_EXECUTABLE(${PROGNAME} ${ALL_STUFF})
ENDIF( APPLE )

TARGET_LINK_LIBRARIES(${PROGNAME} ${LIBS})

INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR})
